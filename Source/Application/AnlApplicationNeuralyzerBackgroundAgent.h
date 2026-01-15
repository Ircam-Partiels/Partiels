#pragma once

#include "../Plugin/AnlPluginListModel.h"
#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationNeuralyzerAgentRemote.h"
#include "AnlApplicationNeuralyzerMcp.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief Runs the agent of the selected backend on a background thread
        //! @details The actions are queued and performed one after the other by the agent of the backend selected
        //! when they are performed. The model is initialized when the backend or the model information of the
        //! accessor changes. The documentation is reindexed when the plugins available on internet change.
        //! A change message is sent when the state changes (from any thread).
        class BackgroundAgent
        : public juce::ChangeBroadcaster
        , private juce::AsyncUpdater
        {
        public:
            // clang-format off
            enum class Action
            {
                  none
                , setupSystem
                , initializeModel
                , initializeRag
                , updateRagIndex
                , sendQuery
                , startSession
                , loadSession
                , saveSession
            };

            enum class StartupDecision
            {
                  wait
                , launch
                , ignore
            };
            // clang-format on

            //! @brief A question asked by the model to the user (see the MCP tool elicit)
            struct Elicitation
            {
                juce::String message;      //!< The question
                juce::StringArray options; //!< The answers suggested by the model

                bool operator==(Elicitation const& rhs) const = default;
            };

            //! @brief Creates the agent and starts the background thread
            //! @details If the engine crashed during the previous session, the user is asked whether the agent should
            //! be used and the background thread waits for the decision.
            //! @param accessor The accessor of the Neuralyzer model
            //! @param mcpDispatcher The dispatcher of the MCP tools and resources
            //! @param pluginListAccessor The accessor of the plugin list (its web references are indexed with the MCP resources)
            BackgroundAgent(Accessor& accessor, Mcp::Dispatcher& mcpDispatcher, PluginList::Accessor& pluginListAccessor);
            //! @brief Aborts the running action and stops the background thread
            ~BackgroundAgent() override;

            //! @brief Gets the accessor of the Neuralyzer model
            Accessor const& getAccessor() const;
            //! @brief Gets the accessor of the Neuralyzer model
            Accessor& getAccessor();
            //! @brief Gets the messages of the current session (thread-safe)
            //! @return The messages or an empty vector without backend
            std::vector<common_chat_msg> getHistory() const;
            //! @brief Gets the response being generated (thread-safe)
            juce::String getTemporaryResponse() const;
            //! @brief Gets the ratio of the context used by the current session (thread-safe)
            //! @return The ratio between 0 and 1
            float getContextCapacityUsage() const;
            //! @brief Gets the information of the initialized model (thread-safe)
            //! @return The information or an invalid one if no model is initialized
            ModelInfo getModelInfo() const;

            //! @brief Checks whether actions are waiting to be performed (thread-safe)
            bool hasPendingAction() const;
            //! @brief Gets the action being performed (thread-safe)
            Action getCurrentAction() const;
            //! @brief Gets the result of the last action (thread-safe)
            //! @details The result is reset when an action starts.
            juce::Result getLastResult() const;
            //! @brief Gets the decision of the user if the engine crashed during the previous session (thread-safe)
            StartupDecision getStartupDecision() const;

            //! @brief Queues a query
            //! @param prompt The prompt of the user
            //! @param useThinkingMode Whether the model should reason before answering
            void sendQuery(juce::String const& prompt, bool useThinkingMode);
            //! @brief Queues the start of a new session (it replaces a pending session action)
            void startSession();
            //! @brief Queues the loading of a session (it replaces a pending session action)
            //! @param sessionFile The file of the session
            void loadSession(juce::File const& sessionFile);
            //! @brief Queues the saving of the current session
            //! @param sessionFile The file of the session
            void saveSession(juce::File const& sessionFile);
            //! @brief Aborts the running query and removes the pending ones
            void cancelQuery();
            //! @brief Reloads the RAG models and the documentation index (for instance after downloading the models)
            void reloadRagModels();
            //! @brief Reindexes the documentation without reloading the RAG models (for instance when the MCP resources change)
            //! @details The action is ignored if the RAG models are not initialized or if the RAG initialization is pending.
            void updateRagIndex();
            //! @brief Gets the engine that searches the documentation (its methods are thread-safe)
            Rag::Engine& getRagEngine();

            //! @brief Gets the question of the model waiting for the answer of the user (thread-safe)
            //! @details The query is suspended until the question is answered or the query is cancelled.
            std::optional<Elicitation> getPendingElicitation() const;
            //! @brief Answers the pending question of the model and resumes the query (thread-safe)
            //! @param answer The answer of the user (ignored without pending question)
            void answerElicitation(juce::String const& answer);

        private:
            struct PendingAction
            {
                Action action{Action::none};
                //! @brief The function performed with the agent of the backend selected when the action is performed
                std::function<juce::Result(Agent&)> fn;
            };

            //! @brief Queues an action and wakes up the background thread
            //! @param pending The action
            //! @param replaceableActions The actions replaced by the new one if one of them is the last pending action
            void enqueue(PendingAction pending, std::initializer_list<Action> replaceableActions = {});
            //! @brief Queues the initialization of the model (it aborts the running initialization and replaces the pending one)
            //! @details The current session is restored in the new model unless a session action is pending.
            void initializeModel(ModelInfo const& info);
            //! @brief Loads the RAG models and indexes the documentation (called on the background thread)
            juce::Result initializeRagEngine();
            //! @brief Indexes the MCP resources and the context documents (called on the background thread)
            juce::Result indexRagResources();
            //! @brief Performs a pending action with the current agent (called on the background thread)
            juce::Result performAction(PendingAction const& pendingAction);
            //! @brief Asks a question to the user and waits for the answer (called on the background thread)
            //! @param agent The agent that performs the query
            //! @param message The question
            //! @param options The answers suggested by the model
            //! @return The answer or nothing if the query is aborted
            std::optional<std::string> elicit(Agent const& agent, std::string const& message, std::vector<std::string> const& options);

            std::optional<std::reference_wrapper<Agent const>> getCurrentAgent() const;

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};

            PluginList::Accessor& mPluginListAccessor;
            PluginList::Accessor::Listener mPluginListListener{typeid(*this).name()};

            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine mRagEngine;
            bool mRagModelsInitialized{false}; //!< Only accessed on the background thread
            std::atomic<AgentBackend> mBackend;
            AgentLocal mAgentLocal;
            AgentRemote mAgentRemote;

            mutable std::mutex mPendingMutex;
            std::condition_variable mPendingCondition;
            std::deque<PendingAction> mPendingActions;

            mutable std::mutex mResultMutex;
            juce::Result mLastResult{juce::Result::ok()};

            std::mutex mStartupMutex;
            std::condition_variable mStartupCondition;
            std::atomic<StartupDecision> mStartupDecision{StartupDecision::wait};

            mutable std::mutex mElicitationMutex;
            std::condition_variable mElicitationCondition;
            std::optional<Elicitation> mPendingElicitation;
            std::optional<juce::String> mElicitationAnswer;

            std::thread mWorkerThread;
            std::atomic<bool> mShouldExit{false};
            std::atomic<Action> mCurrentAction{Action::none};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackgroundAgent)
            JUCE_DECLARE_WEAK_REFERENCEABLE(BackgroundAgent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
