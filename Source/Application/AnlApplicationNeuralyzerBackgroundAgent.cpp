#include "AnlApplicationNeuralyzerBackgroundAgent.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::BackgroundAgent::BackgroundAgent(Accessor& accessor, Mcp::Dispatcher& mcpDispatcher)
: mAccessor(accessor)
, mMcpDispatcher(mcpDispatcher)
, mRagEngine(mMcpDispatcher)
, mAgentLocal(mMcpDispatcher, mRagEngine)
, mAgentRemote(mMcpDispatcher, mRagEngine)
{
    mAgentLocal.setNotifyCallback([this]()
                                  {
                                      sendChangeMessage();
                                  });
    mAgentRemote.setNotifyCallback([this]()
                                   {
                                       sendChangeMessage();
                                   });
    mWorkerThread = std::thread([this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::BackgroundAgent");
                                    mCurrentAction.store(Action::setupSystem);
                                    sendChangeMessage();
                                    initializeEngine();
                                    {
                                        auto result = mRagEngine.initializeModels(Neuralyzer::Rag::getEmbeddingModelFile(), Neuralyzer::Rag::getRerankerModelFile());
                                        std::unique_lock<std::mutex> lock(mResultMutex);
                                        mLastResult = result;
                                    }
                                    mCurrentAction.store(Action::none);
                                    sendChangeMessage();
                                    while(true)
                                    {
                                        PendingAction pendingAction;
                                        {
                                            std::unique_lock<std::mutex> lock(mPendingMutex);
                                            mPendingCondition.wait(lock, [this]
                                                                   {
                                                                       return !mPendingActions.empty() || mShouldExit.load();
                                                                   });
                                            if(mShouldExit.load())
                                            {
                                                break;
                                            }
                                            pendingAction = std::move(mPendingActions.front());
                                            mPendingActions.pop_front();
                                        }

                                        mCurrentAction.store(pendingAction.action);
                                        mAgentLocal.setShouldQuit(false);
                                        mAgentRemote.setShouldQuit(false);
                                        switch(mBackend.load())
                                        {
                                            case AgentBackend::none:
                                            {
                                                mAgentLocal.resetModel();
                                                mAgentRemote.resetModel();
                                                {
                                                    std::unique_lock<std::mutex> lock(mResultMutex);
                                                    mLastResult = juce::Result::ok();
                                                }
                                                mCurrentAction.store(Action::none);
                                                sendChangeMessage();
                                                return;
                                            }
                                            case AgentBackend::local:
                                            {
                                                mAgentRemote.resetModel();
                                                break;
                                            }
                                            case AgentBackend::remote:
                                            {
                                                mAgentLocal.resetModel();
                                                break;
                                            }
                                        }
                                        sendChangeMessage();
                                        MiscDebug("Neuralyzer::BackgroundAgent", "Action: " + std::string(magic_enum::enum_name(pendingAction.action)));
                                        auto result = [&]
                                        {
                                            try
                                            {
                                                return pendingAction.fn();
                                            }
                                            catch(std::exception const& exception)
                                            {
                                                return juce::Result::fail(juce::translate("Unhandled exception: ERROR").replace("ERROR", juce::String(juce::CharPointer_UTF8(exception.what()))));
                                            }
                                            catch(...)
                                            {
                                                return juce::Result::fail(juce::translate("Unhandled unknown exception."));
                                            }
                                        }();
                                        // Keep the latest action result available for UI/state observers.
                                        {
                                            std::unique_lock<std::mutex> lock(mResultMutex);
                                            mLastResult = result;
                                        }
                                        mCurrentAction.store(Action::none);
                                        sendChangeMessage();
                                    }
                                    mAgentLocal.resetModel();
                                    mAgentRemote.resetModel();
                                    releaseEngine();
                                });

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::agentBackend:
            {
                mBackend.store(acsr.getAttr<AttrType::agentBackend>());
                if(acsr.getAttr<AttrType::modelInfo>().isValid())
                {
                    initializeModel(acsr.getAttr<AttrType::modelInfo>());
                }
                sendChangeMessage();
                break;
            }
            case AttrType::modelInfo:
            {
                if(acsr.getAttr<AttrType::modelInfo>().isValid())
                {
                    initializeModel(acsr.getAttr<AttrType::modelInfo>());
                }
                sendChangeMessage();
                break;
            }
            case AttrType::effectiveState:
            case AttrType::mcpForClaudeApp:
            case AttrType::mcpForCopilotApp:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::Neuralyzer::BackgroundAgent::~BackgroundAgent()
{
    mAccessor.removeListener(mListener);
    mAgentLocal.setShouldQuit(true);
    mAgentRemote.setShouldQuit(true);
    mShouldExit.store(true);
    {
        std::unique_lock<std::mutex> lock(mPendingMutex);
        mPendingActions.clear();
    }
    mPendingCondition.notify_all();
    if(mWorkerThread.joinable())
    {
        mWorkerThread.join();
    }
}

std::optional<std::reference_wrapper<Application::Neuralyzer::Agent>> Application::Neuralyzer::BackgroundAgent::getCurrentAgent()
{
    switch(mBackend.load())
    {
        case AgentBackend::none:
        {
            return {};
        }
        case AgentBackend::local:
        {
            return mAgentLocal;
        }
        case AgentBackend::remote:
        {
            return mAgentRemote;
        }
    }
    return {};
}

std::optional<std::reference_wrapper<Application::Neuralyzer::Agent const>> Application::Neuralyzer::BackgroundAgent::getCurrentAgent() const
{
    switch(mBackend.load())
    {
        case AgentBackend::none:
        {
            return {};
        }
        case AgentBackend::local:
        {
            return mAgentLocal;
        }
        case AgentBackend::remote:
        {
            return mAgentRemote;
        }
    }
    return {};
}

Application::Neuralyzer::Accessor const& Application::Neuralyzer::BackgroundAgent::getAccessor() const
{
    return mAccessor;
}

Application::Neuralyzer::Agent::History Application::Neuralyzer::BackgroundAgent::getHistory() const
{
    auto const optAgent = getCurrentAgent();
    return optAgent.has_value() ? optAgent.value().get().getHistory() : Agent::History{};
}

juce::String Application::Neuralyzer::BackgroundAgent::getTemporaryResponse() const
{
    auto const optAgent = getCurrentAgent();
    return optAgent.has_value() ? optAgent.value().get().getTemporaryResponse() : juce::String{};
}

float Application::Neuralyzer::BackgroundAgent::getContextCapacityUsage() const
{
    auto const optAgent = getCurrentAgent();
    return optAgent.has_value() ? optAgent.value().get().getContextCapacityUsage() : 0.0f;
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::BackgroundAgent::getModelInfo() const
{
    auto const optAgent = getCurrentAgent();
    return optAgent.has_value() ? optAgent.value().get().getModelInfo() : ModelInfo{};
}

void Application::Neuralyzer::BackgroundAgent::initializeModel(ModelInfo const& info)
{
    auto optAgent = getCurrentAgent();
    if(!optAgent.has_value())
    {
        return;
    }
    PendingAction pending{Action::initializeModel,
                          [this, info, &agent = optAgent.value().get()]() -> juce::Result
                          {
                              auto const tempDirectory = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory);
                              auto const tempSessionFile = getSessionFile(tempDirectory.getChildFile("neuralyzersession.ptldoc"));

                              // Save the current session history to restore it after the model initialization.
                              auto const saveResult = [&]()
                              {
                                  // If a session load or start is pending, we skip saving and restoring.
                                  {
                                      std::unique_lock<std::mutex> lock(mPendingMutex);
                                      if(!mPendingActions.empty() && (mPendingActions.back().action == Action::loadSession || mPendingActions.back().action == Action::startSession))
                                      {
                                          return juce::Result::fail("Delayed");
                                      }
                                  }
                                  // If the current model is valid, we save the session to restore it after initializing the new model. If not, we skip saving and restoring since there's no session to keep.
                                  if(agent.getModelInfo().isValid())
                                  {
                                      return agent.saveSession(tempSessionFile);
                                  }
                                  return juce::Result::fail("No session");
                              }();
                              mCurrentAction.store(Action::initializeModel);
                              auto const initializeResult = agent.initializeModel(info);
                              if(initializeResult.failed())
                              {
                                  return initializeResult;
                              }
                              triggerAsyncUpdate();
                              // If a session load or start is pending, we skip loading or restoring the session.
                              {
                                  std::unique_lock<std::mutex> lock(mPendingMutex);
                                  if(!mPendingActions.empty() && (mPendingActions.back().action == Action::loadSession || mPendingActions.back().action == Action::startSession))
                                  {
                                      return juce::Result::ok();
                                  }
                              }
                              // If no session is stored, we start a new session
                              if(saveResult.failed())
                              {
                                  mCurrentAction.store(Action::startSession);
                                  sendChangeMessage();
                                  return agent.startSession();
                              }
                              // If a session is stored, we load it to restore the history in the new model. If loading fails, we start a new session.
                              mCurrentAction.store(Action::loadSession);
                              sendChangeMessage();
                              if(agent.loadSession(tempSessionFile).failed())
                              {
                                  mCurrentAction.store(Action::startSession);
                                  sendChangeMessage();
                                  return agent.startSession();
                              }
                              return juce::Result::ok();
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mCurrentAction.load() == Action::initializeModel)
    {
        // A model load is already running: abort it so the new request starts promptly.
        // The worker resets mShouldQuit to false before executing the next action.
        mAgentLocal.setShouldQuit(true);
        mAgentRemote.setShouldQuit(true);
    }
    if(!mPendingActions.empty() && mPendingActions.back().action == Action::initializeModel)
    {
        mPendingActions.back() = std::move(pending);
    }
    else
    {
        mPendingActions.push_back(std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::sendQuery(juce::String const& prompt, nlohmann::json const& mcpToolsContext)
{
    auto optAgent = getCurrentAgent();
    if(!optAgent.has_value())
    {
        return;
    }
    PendingAction pending{Action::sendQuery,
                          [this, prompt, mcpToolsContext, &agent = optAgent.value().get()]() -> juce::Result
                          {
                              mMcpDispatcher.setContext(mcpToolsContext);
                              return agent.sendQuery(prompt);
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    mPendingActions.push_back(std::move(pending));
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::startSession()
{
    auto optAgent = getCurrentAgent();
    if(!optAgent.has_value())
    {
        return;
    }
    PendingAction pending{Action::startSession,
                          [&agent = optAgent.value().get()]() -> juce::Result
                          {
                              return agent.startSession();
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(!mPendingActions.empty() && (mPendingActions.back().action == Action::startSession || mPendingActions.back().action == Action::loadSession))
    {
        mPendingActions.back() = std::move(pending);
    }
    else
    {
        mPendingActions.push_back(std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::loadSession(juce::File const sessionFile)
{
    auto optAgent = getCurrentAgent();
    if(!optAgent.has_value())
    {
        return;
    }
    PendingAction pending{Action::loadSession,
                          [=, &agent = optAgent.value().get()]() -> juce::Result
                          {
                              return agent.loadSession(sessionFile);
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(!mPendingActions.empty() && (mPendingActions.back().action == Action::loadSession || mPendingActions.back().action == Action::startSession))
    {
        mPendingActions.back() = std::move(pending);
    }
    else
    {
        mPendingActions.push_back(std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::saveSession(juce::File const sessionFile)
{
    auto optAgent = getCurrentAgent();
    if(!optAgent.has_value())
    {
        return;
    }
    PendingAction pending{Action::saveSession,
                          [=, &agent = optAgent.value().get()]() -> juce::Result
                          {
                              return agent.saveSession(sessionFile);
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    mPendingActions.push_back(std::move(pending));
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::cancelQuery()
{
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mCurrentAction.load() == Action::sendQuery)
    {
        // Signal the running action to abort. The worker thread resets
        // mShouldQuit to false before starting the next action.
        if(mAgentLocal.getModelInfo().isValid())
        {
            mAgentLocal.setShouldQuit(true);
        }
        if(mAgentRemote.getModelInfo().isValid())
        {
            mAgentRemote.setShouldQuit(true);
        }
    }
    mPendingActions.erase(std::remove_if(mPendingActions.begin(), mPendingActions.end(),
                                         [](auto const& pa)
                                         {
                                             return pa.action == Action::sendQuery;
                                         }),
                          mPendingActions.end());
    lock.unlock();
    mPendingCondition.notify_one();
}

bool Application::Neuralyzer::BackgroundAgent::hasPendingAction() const
{
    std::unique_lock<std::mutex> lock(mPendingMutex);
    return !mPendingActions.empty();
}

Application::Neuralyzer::BackgroundAgent::Action Application::Neuralyzer::BackgroundAgent::getCurrentAction() const
{
    return mCurrentAction.load();
}

juce::Result Application::Neuralyzer::BackgroundAgent::getLastResult() const
{
    std::unique_lock<std::mutex> lock(mResultMutex);
    return mLastResult;
}

void Application::Neuralyzer::BackgroundAgent::handleAsyncUpdate()
{
    mAccessor.setAttr<AttrType::effectiveState>(getModelInfo(), NotificationType::synchronous);
    sendSynchronousChangeMessage();
}

ANALYSE_FILE_END
