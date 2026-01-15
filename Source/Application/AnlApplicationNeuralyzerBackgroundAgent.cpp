#include "AnlApplicationNeuralyzerBackgroundAgent.h"
#include "AnlApplicationNeuralyzerTools.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::BackgroundAgent::BackgroundAgent(Mcp::Dispatcher& mcpDispatcher, juce::String const& instructions, juce::String const& firstQuery)
: mAgent(mcpDispatcher)
{
    mAgent.setInstructions(instructions, firstQuery);
    mAgent.setNotifyCallback([this]()
                             {
                                 sendChangeMessage();
                             });
    mWorkerThread = std::thread([this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::BackgroundAgent");
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
                                        mAgent.setShouldQuit(false);
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
                                                return juce::Result::fail(juce::translate("Unhandled exception: ERROR").replace("ERROR", juce::String(exception.what())));
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
    mAgent.setShouldQuit(true);
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

Application::Neuralyzer::Agent::History Application::Neuralyzer::BackgroundAgent::getHistory() const
{
    return mAgent.getHistory();
}

juce::String Application::Neuralyzer::BackgroundAgent::getTemporaryResponse() const
{
    return mAgent.getTemporaryResponse();
}

float Application::Neuralyzer::BackgroundAgent::getContextCapacityUsage() const
{
    return mAgent.getContextCapacityUsage();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::BackgroundAgent::getModelInfo() const
{
    return mAgent.getModelInfo();
}

void Application::Neuralyzer::BackgroundAgent::initializeModel(ModelInfo const& info)
{
    PendingAction pending{Action::initializeModel,
                          [this, info]() -> juce::Result
                          {
                              static auto const tempSessionFiles = getNeuralyzerSessionFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("neuralyzersession.ptldoc"));
                              auto const saveResult = [&]()
                              {
                                  if(mAgent.getModelInfo().isValid())
                                  {
                                      mAgent.setSessionFiles(std::get<0>(tempSessionFiles), std::get<1>(tempSessionFiles));
                                      return mAgent.saveSession();
                                  }
                                  return juce::Result::ok();
                              }();
                              auto const initializeResult = mAgent.initializeModel(info);
                              if(initializeResult.wasOk())
                              {
                                  if(saveResult.failed())
                                  {
                                      return saveResult;
                                  }
                                  auto const loadResult = mAgent.loadSession();
                                  std::get<0>(tempSessionFiles).deleteFile();
                                  std::get<1>(tempSessionFiles).deleteFile();
                                  return loadResult;
                              }
                              return initializeResult;
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mCurrentAction.load() == Action::initializeModel)
    {
        // A model load is already running: abort it so the new request starts promptly.
        // The worker resets mShouldQuit to false before executing the next action.
        mAgent.setShouldQuit(true);
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

void Application::Neuralyzer::BackgroundAgent::sendQuery(juce::String const& prompt)
{
    PendingAction pending{Action::sendQuery,
                          [this, prompt]() -> juce::Result
                          {
                              return mAgent.sendQuery(prompt);
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
                              return mAgent.startSession();
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

void Application::Neuralyzer::BackgroundAgent::loadSession(juce::File const contextFile, juce::File const messageFile)
{
    PendingAction pending{Action::loadSession,
                          [=, this]() -> juce::Result
                          {
                              mAgent.setSessionFiles(contextFile, messageFile);
                              return mAgent.loadSession();
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

void Application::Neuralyzer::BackgroundAgent::saveSession(juce::File const contextFile, juce::File const messageFile)
{
    PendingAction pending{Action::saveSession,
                          [=, this]() -> juce::Result
                          {
                              mAgent.setSessionFiles(contextFile, messageFile);
                              return mAgent.saveSession();
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    mPendingActions.push_back(std::move(pending));
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::cancelQuery()
{
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mAgent.getModelInfo().isValid() && mCurrentAction.load() == Action::sendQuery)
    {
        // Signal the running action to abort. The worker thread resets
        // mShouldQuit to false before starting the next action.
        mAgent.setShouldQuit(true);
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
