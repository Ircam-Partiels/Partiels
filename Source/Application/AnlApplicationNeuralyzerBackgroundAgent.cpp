#include "AnlApplicationNeuralyzerBackgroundAgent.h"

ANALYSE_FILE_BEGIN

namespace
{
    //! @brief The temporary file used to keep the session while the model is initialized
    juce::File getModelSessionFile()
    {
        return Application::Neuralyzer::getTempSessionFile("neuralyzersession.ptldoc");
    }

    //! @brief Deletes a session file and the context state saved with it
    void deleteSessionFile(juce::File const& sessionFile)
    {
        sessionFile.deleteFile();
        Application::Neuralyzer::EngineLocal::deleteState(sessionFile.withFileExtension(".ctx"));
    }

    bool isSessionAction(Application::Neuralyzer::BackgroundAgent::Action action)
    {
        return action == Application::Neuralyzer::BackgroundAgent::Action::loadSession || action == Application::Neuralyzer::BackgroundAgent::Action::startSession;
    }
} // namespace

Application::Neuralyzer::BackgroundAgent::BackgroundAgent(Accessor& accessor, Mcp::Dispatcher& mcpDispatcher, PluginList::Accessor& pluginListAccessor)
: mAccessor(accessor)
, mPluginListAccessor(pluginListAccessor)
, mMcpDispatcher(mcpDispatcher)
, mBackend(accessor.getAttr<AttrType::agentBackend>())
, mAgentLocal(mMcpDispatcher, mRagEngine)
, mAgentRemote(mMcpDispatcher, mRagEngine)
{
    mAgentLocal.setNotifyCallback([this]()
                                  {
                                      sendChangeMessage();
                                  });
    mAgentLocal.setThinkingMode(accessor.getAttr<AttrType::enableThinking>());
    mAgentRemote.setNotifyCallback([this]()
                                   {
                                       sendChangeMessage();
                                   });
    mAgentLocal.setElicitCallback([this](std::string const& message, std::vector<std::string> const& options)
                                  {
                                      return elicit(mAgentLocal, message, options);
                                  });
    mAgentRemote.setElicitCallback([this](std::string const& message, std::vector<std::string> const& options)
                                   {
                                       return elicit(mAgentRemote, message, options);
                                   });

    // A GGML abort file from a previous session means the engine crashed: ask the user
    // whether Neuralyzer should be used anyway or disabled, and block the worker thread
    // until the decision is made (the engine and the RAG models are not initialized yet).
    auto const abortFile = getAbortFile();
    if(abortFile.existsAsFile())
    {
        auto const errorMessage = abortFile.loadFileAsString();
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("The agent crashed during the previous session"))
                                 .withMessage(juce::translate("The agent engine crashed during the previous session with the following error: ERRSTR. Do you want to keep using the agent or disable it?").replace("ERRSTR", errorMessage))
                                 .withButton(juce::translate("Use the agent"), 1)
                                 .withButton(juce::translate("Disable the agent"), 0);

        juce::WeakReference<BackgroundAgent> weakReference(this);
        juce::AlertWindow::showAsync(options, [weakReference](int result)
                                     {
                                         auto* agent = weakReference.get();
                                         if(agent == nullptr)
                                         {
                                             return;
                                         }
                                         if(result == 1)
                                         {
                                             // The user acknowledged the previous crash: clear the abort file.
                                             getAbortFile().deleteFile();
                                         }
                                         {
                                             std::unique_lock<std::mutex> lock(agent->mStartupMutex);
                                             agent->mStartupDecision.store(result == 1 ? StartupDecision::launch : StartupDecision::ignore);
                                         }
                                         agent->mStartupCondition.notify_all();
                                         agent->sendChangeMessage();
                                     });
    }
    else
    {
        std::unique_lock<std::mutex> lock(mStartupMutex);
        mStartupDecision.store(StartupDecision::launch);
    }

    mWorkerThread = std::thread([this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::BackgroundAgent");
                                    // Wait for the user's decision if a GGML abort file was found at startup.
                                    {
                                        std::unique_lock<std::mutex> lock(mStartupMutex);
                                        mStartupCondition.wait(lock, [this]
                                                               {
                                                                   return mStartupDecision.load() != StartupDecision::wait || mShouldExit.load();
                                                               });
                                    }
                                    if(mShouldExit.load() || mStartupDecision.load() == StartupDecision::ignore)
                                    {
                                        // Nothing is initialized: leave the thread immediately.
                                        return;
                                    }
                                    mCurrentAction.store(Action::setupSystem);
                                    sendChangeMessage();
                                    // A session file left by a crash during a model initialization must not be restored.
                                    deleteSessionFile(getModelSessionFile());
                                    initializeEngine();

                                    auto const ragInitResult = initializeRagEngine();
                                    if(ragInitResult.failed())
                                    {
                                        std::unique_lock<std::mutex> lock(mResultMutex);
                                        mLastResult = ragInitResult;
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
                                            // The current action and the quit flags are updated with the lock so that a
                                            // cancellation requested by cancelQuery() or initializeModel() cannot be missed or overwritten.
                                            mCurrentAction.store(pendingAction.action);
                                            mAgentLocal.setShouldQuit(false);
                                            mAgentRemote.setShouldQuit(false);
                                        }

                                        {
                                            std::unique_lock<std::mutex> lock(mResultMutex);
                                            mLastResult = juce::Result::ok();
                                        }
                                        sendChangeMessage();
                                        MiscDebug("Neuralyzer::BackgroundAgent", "Action: " + std::string(magic_enum::enum_name(pendingAction.action)));
                                        auto result = [&]
                                        {
                                            try
                                            {
                                                return performAction(pendingAction);
                                            }
                                            catch(std::exception const& exception)
                                            {
                                                return juce::Result::fail(juce::translate("Unhandled exception: ERRSTR").replace("ERRSTR", juce::String(juce::CharPointer_UTF8(exception.what()))));
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
                // Without backend, the action releases the models (see performAction())
                if(mBackend.load() == AgentBackend::none || acsr.getAttr<AttrType::modelInfo>().isValid())
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
            case AttrType::enableThinking:
            {
                mAgentLocal.setThinkingMode(acsr.getAttr<AttrType::enableThinking>());
                sendChangeMessage();
                break;
            }
            case AttrType::effectiveState:
            case AttrType::mcpForClaudeApp:
            case AttrType::mcpForCopilotApp:
            case AttrType::mcpForLlamaCpp:
            case AttrType::showAdvancedSettings:
            case AttrType::ragModelsDownload:
            case AttrType::localModelsDownload:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);

    // The plugins available on internet are listed in an MCP resource indexed by the RAG engine
    mPluginListListener.onAttrChanged = [this]([[maybe_unused]] PluginList::Accessor const& acsr, PluginList::AttrType attr)
    {
        switch(attr)
        {
            case PluginList::AttrType::webReferences:
            {
                updateRagIndex();
                break;
            }
            case PluginList::AttrType::useEnvVariable:
            case PluginList::AttrType::quarantineMode:
            case PluginList::AttrType::searchPath:
            case PluginList::AttrType::includePreReleases:
                break;
        }
    };
    mPluginListAccessor.addListener(mPluginListListener, NotificationType::synchronous);
}

Application::Neuralyzer::BackgroundAgent::~BackgroundAgent()
{
    mPluginListAccessor.removeListener(mPluginListListener);
    mAccessor.removeListener(mListener);
    mAgentLocal.setShouldQuit(true);
    mAgentRemote.setShouldQuit(true);
    mRagEngine.setShouldQuit(true);
    mShouldExit.store(true);
    {
        std::unique_lock<std::mutex> lock(mPendingMutex);
        mPendingActions.clear();
    }
    mPendingCondition.notify_all();
    {
        // Guarantees that the worker is either before the predicate check or blocked in the wait
        // (mShouldExit is part of the wait predicate), so the notification cannot be missed.
        std::unique_lock<std::mutex> lock(mStartupMutex);
    }
    mStartupCondition.notify_all();
    if(mWorkerThread.joinable())
    {
        mWorkerThread.join();
    }
}

Application::Neuralyzer::BackgroundAgent::StartupDecision Application::Neuralyzer::BackgroundAgent::getStartupDecision() const
{
    return mStartupDecision.load();
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

Application::Neuralyzer::Accessor& Application::Neuralyzer::BackgroundAgent::getAccessor()
{
    return mAccessor;
}

std::vector<common_chat_msg> Application::Neuralyzer::BackgroundAgent::getHistory() const
{
    auto const optAgent = getCurrentAgent();
    return optAgent.has_value() ? optAgent.value().get().getHistory() : std::vector<common_chat_msg>{};
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

juce::Result Application::Neuralyzer::BackgroundAgent::performAction(PendingAction const& pendingAction)
{
    // The RAG engine is independent of the agent backend
    if(pendingAction.action == Action::initializeRag)
    {
        return initializeRagEngine();
    }
    if(pendingAction.action == Action::updateRagIndex)
    {
        return indexRagResources();
    }
    // The agent is resolved when the action is performed so that the actions queued
    // before a change of backend are performed by the agent of the new backend.
    switch(mBackend.load())
    {
        case AgentBackend::none:
        {
            // The action is dropped and the models are released
            mAgentLocal.resetModel();
            mAgentRemote.resetModel();
            triggerAsyncUpdate();
            return juce::Result::ok();
        }
        case AgentBackend::local:
        {
            mAgentRemote.resetModel();
            return pendingAction.fn(mAgentLocal);
        }
        case AgentBackend::remote:
        {
            mAgentLocal.resetModel();
            return pendingAction.fn(mAgentRemote);
        }
    }
    return juce::Result::ok();
}

void Application::Neuralyzer::BackgroundAgent::initializeModel(ModelInfo const& info)
{
    PendingAction pending{Action::initializeModel,
                          [this, info](Agent& agent) -> juce::Result
                          {
                              // The parameters that do not require to reload the model are applied directly: the
                              // session is kept without being saved and restored.
                              if(agent.canUpdateModel(info))
                              {
                                  auto const updateResult = agent.updateModel(info);
                                  triggerAsyncUpdate();
                                  return updateResult;
                              }

                              auto const hasPendingSession = [this]()
                              {
                                  std::unique_lock<std::mutex> lock(mPendingMutex);
                                  return std::any_of(mPendingActions.cbegin(), mPendingActions.cend(), [](auto const& pa)
                                                     {
                                                         return isSessionAction(pa.action);
                                                     });
                              };

                              // The session is saved to be restored after the model initialization. If the model is not
                              // initialized (because the previous initialization was aborted or failed), the session file
                              // saved by the previous initialization is kept.
                              auto const sessionFile = getModelSessionFile();
                              if(hasPendingSession())
                              {
                                  // A pending session replaces the current one: there is nothing to restore.
                                  deleteSessionFile(sessionFile);
                              }
                              else if(agent.getModelInfo().isValid() && agent.saveSession(sessionFile).failed())
                              {
                                  deleteSessionFile(sessionFile);
                              }

                              auto const initializeResult = agent.initializeModel(info);
                              // The effective state is updated even if the initialization failed.
                              triggerAsyncUpdate();
                              if(initializeResult.failed())
                              {
                                  return initializeResult;
                              }
                              if(hasPendingSession())
                              {
                                  deleteSessionFile(sessionFile);
                                  return juce::Result::ok();
                              }
                              // The saved session is restored in the new model. If there is no session or if the
                              // loading fails, a new session is started.
                              if(sessionFile.existsAsFile())
                              {
                                  mCurrentAction.store(Action::loadSession);
                                  sendChangeMessage();
                                  auto const loadResult = agent.loadSession(sessionFile);
                                  deleteSessionFile(sessionFile);
                                  if(loadResult.wasOk())
                                  {
                                      return loadResult;
                                  }
                              }
                              mCurrentAction.store(Action::startSession);
                              sendChangeMessage();
                              return agent.startSession();
                          }};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mCurrentAction.load() == Action::initializeModel)
    {
        // A model load is already running: abort it so the new request starts promptly.
        // The worker resets mShouldQuit to false before executing the next action.
        mAgentLocal.setShouldQuit(true);
        mAgentRemote.setShouldQuit(true);
    }
    auto it = std::find_if(mPendingActions.begin(), mPendingActions.end(), [](auto const& pa)
                           {
                               return pa.action == Action::initializeModel;
                           });
    if(it != mPendingActions.end())
    {
        *it = std::move(pending);
    }
    else
    {
        // The model is initialized before the pending session actions that require it.
        it = std::find_if(mPendingActions.begin(), mPendingActions.end(), [](auto const& pa)
                          {
                              return isSessionAction(pa.action);
                          });
        mPendingActions.insert(it, std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::enqueue(PendingAction pending, std::initializer_list<Action> replaceableActions)
{
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(!mPendingActions.empty() && std::find(replaceableActions.begin(), replaceableActions.end(), mPendingActions.back().action) != replaceableActions.end())
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

void Application::Neuralyzer::BackgroundAgent::sendQuery(juce::String const& prompt, bool useThinkingMode)
{
    if(mBackend.load() == AgentBackend::none)
    {
        return;
    }
    enqueue({Action::sendQuery, [prompt, useThinkingMode](Agent& agent)
             {
                 return agent.sendQuery(prompt, useThinkingMode);
             }});
}

void Application::Neuralyzer::BackgroundAgent::startSession()
{
    if(mBackend.load() == AgentBackend::none)
    {
        return;
    }
    enqueue({Action::startSession, [](Agent& agent)
             {
                 return agent.startSession();
             }},
            {Action::startSession, Action::loadSession});
}

void Application::Neuralyzer::BackgroundAgent::loadSession(juce::File const& sessionFile)
{
    if(mBackend.load() == AgentBackend::none)
    {
        return;
    }
    enqueue({Action::loadSession, [sessionFile](Agent& agent)
             {
                 return agent.loadSession(sessionFile);
             }},
            {Action::startSession, Action::loadSession});
}

void Application::Neuralyzer::BackgroundAgent::saveSession(juce::File const& sessionFile)
{
    if(mBackend.load() == AgentBackend::none)
    {
        return;
    }
    enqueue({Action::saveSession, [sessionFile](Agent& agent)
             {
                 return agent.saveSession(sessionFile);
             }});
}

void Application::Neuralyzer::BackgroundAgent::cancelQuery()
{
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(mCurrentAction.load() == Action::sendQuery)
    {
        // Signal the running action to abort. The worker thread resets
        // mShouldQuit to false before starting the next action.
        mAgentLocal.setShouldQuit(true);
        mAgentRemote.setShouldQuit(true);
    }
    mPendingActions.erase(std::remove_if(mPendingActions.begin(), mPendingActions.end(),
                                         [](auto const& pa)
                                         {
                                             return pa.action == Action::sendQuery;
                                         }),
                          mPendingActions.end());
}

void Application::Neuralyzer::BackgroundAgent::reloadRagModels()
{
    // The RAG engine is independent of the agent backend: the action is performed by performAction() without agent.
    PendingAction pending{Action::initializeRag, nullptr};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(std::none_of(mPendingActions.cbegin(), mPendingActions.cend(), [](auto const& pa)
                    {
                        return pa.action == Action::initializeRag;
                    }))
    {
        mPendingActions.push_back(std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

void Application::Neuralyzer::BackgroundAgent::updateRagIndex()
{
    // The RAG engine is independent of the agent backend: the action is performed by performAction() without agent.
    // A pending initialization or reindexation of the RAG already indexes the current resources.
    PendingAction pending{Action::updateRagIndex, nullptr};
    std::unique_lock<std::mutex> lock(mPendingMutex);
    if(std::none_of(mPendingActions.cbegin(), mPendingActions.cend(), [](auto const& pa)
                    {
                        return pa.action == Action::initializeRag || pa.action == Action::updateRagIndex;
                    }))
    {
        mPendingActions.push_back(std::move(pending));
    }
    lock.unlock();
    mPendingCondition.notify_one();
}

Application::Neuralyzer::Rag::Engine& Application::Neuralyzer::BackgroundAgent::getRagEngine()
{
    return mRagEngine;
}

std::optional<Application::Neuralyzer::BackgroundAgent::Elicitation> Application::Neuralyzer::BackgroundAgent::getPendingElicitation() const
{
    std::unique_lock<std::mutex> lock(mElicitationMutex);
    // The answered question remains pending until the background thread resumes the query
    return mElicitationAnswer.has_value() ? std::optional<Elicitation>{} : mPendingElicitation;
}

void Application::Neuralyzer::BackgroundAgent::answerElicitation(juce::String const& answer)
{
    {
        std::unique_lock<std::mutex> lock(mElicitationMutex);
        if(!mPendingElicitation.has_value() || mElicitationAnswer.has_value())
        {
            return;
        }
        mElicitationAnswer = answer;
    }
    mElicitationCondition.notify_all();
}

std::optional<std::string> Application::Neuralyzer::BackgroundAgent::elicit(Agent const& agent, std::string const& message, std::vector<std::string> const& options)
{
    Elicitation elicitation;
    elicitation.message = juce::String::fromUTF8(message.c_str()).trim();
    for(auto const& option : options)
    {
        auto const text = juce::String::fromUTF8(option.c_str()).trim();
        if(text.isNotEmpty())
        {
            elicitation.options.addIfNotAlreadyThere(text);
        }
    }
    {
        std::unique_lock<std::mutex> lock(mElicitationMutex);
        mPendingElicitation = std::move(elicitation);
        mElicitationAnswer.reset();
    }
    sendChangeMessage();

    std::optional<std::string> answer;
    {
        std::unique_lock<std::mutex> lock(mElicitationMutex);
        // The quit flags are not notified (see cancelQuery() and the destructor): they are checked periodically
        while(!mElicitationAnswer.has_value() && !agent.shouldQuit() && !mShouldExit.load())
        {
            mElicitationCondition.wait_for(lock, std::chrono::milliseconds(100));
        }
        if(mElicitationAnswer.has_value() && !agent.shouldQuit())
        {
            answer = mElicitationAnswer.value().toStdString();
        }
        mPendingElicitation.reset();
        mElicitationAnswer.reset();
    }
    sendChangeMessage();
    return answer;
}

juce::Result Application::Neuralyzer::BackgroundAgent::initializeRagEngine()
{
    auto const result = mRagEngine.initializeModels(Neuralyzer::Rag::getEmbeddingModelFile(), Neuralyzer::Rag::getRerankerModelFile());
    mRagModelsInitialized = result.wasOk();
    if(result.failed())
    {
        return result;
    }
    return indexRagResources();
}

juce::Result Application::Neuralyzer::BackgroundAgent::indexRagResources()
{
    // Without the RAG models (not downloaded or failed to load), there is nothing to index
    if(!mRagModelsInitialized)
    {
        return juce::Result::ok();
    }
    std::vector<Application::Neuralyzer::Rag::Resource> allResources;
    auto const addResources = [&](juce::String const& text, juce::String const& name)
    {
        auto const ast = Neuralyzer::Ast::parseMarkdown(text, name, false);
        auto const resources = Neuralyzer::Rag::createResources(ast, 1200);
        allResources.insert(allResources.end(), resources.cbegin(), resources.cend());
    };
    for(auto const& [name, text] : mMcpDispatcher.getResources())
    {
        addResources(text, name);
    }

    // The Markdown files added by the user to the context directory
    auto const contextDirectory = Neuralyzer::Rag::getContextDirectory();
    [[maybe_unused]] auto const createResult = contextDirectory.createDirectory();
    MiscWeakAssert(createResult.wasOk());
    for(auto const& [name, text] : Neuralyzer::Rag::getContextDocuments(contextDirectory))
    {
        MiscDebug("Application::Neuralyzer::BackgroundAgent", "Indexing context document: " + name);
        addResources(text, name);
    }
    Neuralyzer::Rag::makeUniqueIdentifiers(allResources);
    return mRagEngine.setResources(allResources);
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
