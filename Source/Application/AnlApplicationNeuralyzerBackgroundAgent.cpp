#include "AnlApplicationNeuralyzerBackgroundAgent.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::BackgroundAgent::BackgroundAgent(Mcp::Dispatcher& mcpDispatcher, std::function<juce::Result(void)> setupSystem)
: mMcpDispatcher(mcpDispatcher)
, mAgentLocal(mcpDispatcher)
, mAgentRemote(mcpDispatcher)
{
    mAgentLocal.setNotifyCallback([this]()
                                  {
                                      sendChangeMessage();
                                  });
    mAgentRemote.setNotifyCallback([this]()
                                   {
                                       sendChangeMessage();
                                   });
    AgentLocal::initialize();
    mWorkerThread = std::thread([this, setupSystemFn = std::move(setupSystem)]()
                                {
                                    mCurrentAction.store(Action::setupSystem);
                                    sendChangeMessage();
                                    juce::Thread::setCurrentThreadName("Neuralyzer::BackgroundAgent");
                                    if(setupSystemFn != nullptr)
                                    {
                                        auto result = setupSystemFn();
                                        {
                                            std::unique_lock<std::mutex> lock(mResultMutex);
                                            mLastResult = result;
                                        }
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
                                        if(mBackend.load() == AgentBackend::local)
                                        {
                                            mAgentRemote.resetModel();
                                        }
                                        else
                                        {
                                            mAgentLocal.resetModel();
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
                                });
}

Application::Neuralyzer::BackgroundAgent::~BackgroundAgent()
{
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
    mAgentLocal.resetModel();
    mAgentRemote.resetModel();
    AgentLocal::release();
}

Application::Neuralyzer::Agent& Application::Neuralyzer::BackgroundAgent::getCurrentAgent()
{
    if(mBackend.load() == AgentBackend::local)
    {
        return mAgentLocal;
    }
    return mAgentRemote;
}

Application::Neuralyzer::Agent const& Application::Neuralyzer::BackgroundAgent::getCurrentAgent() const
{
    if(mBackend.load() == AgentBackend::local)
    {
        return mAgentLocal;
    }
    return mAgentRemote;
}

Application::Neuralyzer::Agent::History Application::Neuralyzer::BackgroundAgent::getHistory() const
{
    return getCurrentAgent().getHistory();
}

juce::String Application::Neuralyzer::BackgroundAgent::getTemporaryResponse() const
{
    return getCurrentAgent().getTemporaryResponse();
}

float Application::Neuralyzer::BackgroundAgent::getContextCapacityUsage() const
{
    return getCurrentAgent().getContextCapacityUsage();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::BackgroundAgent::getModelInfo() const
{
    return getCurrentAgent().getModelInfo();
}

void Application::Neuralyzer::BackgroundAgent::setAgentBackend(AgentBackend backend)
{
    mBackend.store(backend);
}

void Application::Neuralyzer::BackgroundAgent::setFirstQuery(juce::String const& firstQuery)
{
    mAgentLocal.setFirstQuery(firstQuery);
    mAgentRemote.setFirstQuery(firstQuery);
}

void Application::Neuralyzer::BackgroundAgent::initializeModel(ModelInfo const& info)
{
    PendingAction pending{Action::initializeModel,
                          [this, info]() -> juce::Result
                          {
                              auto const tempSessionFile = getNeuralyzerSessionFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("neuralyzersession.ptldoc"));
                              auto& agent = getCurrentAgent();
                              auto const saveResult = [&]()
                              {
                                  if(agent.getModelInfo().isValid())
                                  {
                                      return agent.saveSession(tempSessionFile);
                                  }
                                  return juce::Result::ok();
                              }();
                              auto const initializeResult = agent.initializeModel(info);
                              if(initializeResult.wasOk())
                              {
                                  if(saveResult.failed())
                                  {
                                      mCurrentAction.store(Action::startSession);
                                      sendChangeMessage();
                                      return agent.startSession();
                                  }
                                  mCurrentAction.store(Action::loadSession);
                                  sendChangeMessage();
                                  return agent.loadSession(tempSessionFile);
                              }
                              return initializeResult;
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
    PendingAction pending{Action::sendQuery,
                          [this, prompt, mcpToolsContext]() -> juce::Result
                          {
                              mMcpDispatcher.setContext(mcpToolsContext);
                              return getCurrentAgent().sendQuery(prompt);
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    mPendingActions.push_back(std::move(pending));
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::startSession()
{
    PendingAction pending{Action::startSession,
                          [=, this]() -> juce::Result
                          {
                              return getCurrentAgent().startSession();
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
    PendingAction pending{Action::loadSession,
                          [=, this]() -> juce::Result
                          {
                              return getCurrentAgent().loadSession(sessionFile);
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
    PendingAction pending{Action::saveSession,
                          [=, this]() -> juce::Result
                          {
                              return getCurrentAgent().saveSession(sessionFile);
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

ANALYSE_FILE_END
