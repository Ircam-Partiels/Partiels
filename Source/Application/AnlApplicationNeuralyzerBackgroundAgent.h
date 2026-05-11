#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class BackgroundAgent
        : public juce::ChangeBroadcaster
        {
        public:
            // clang-format off
            enum class Action
            {
                  none
                , setupSystem
                , initializeModel
                , sendQuery
                , startSession
                , loadSession
                , saveSession
                , cancelQuery
            };
            // clang-format on

            BackgroundAgent(Mcp::Dispatcher& mcpDispatcher, std::function<juce::Result(void)> setupSystem);
            ~BackgroundAgent();

            // Getters that are pass through to Agent - safe to call from any thread
            Agent::History getHistory() const;
            juce::String getTemporaryResponse() const;
            float getContextCapacityUsage() const;
            ModelInfo getModelInfo() const;

            // State inspection - safe to call from any thread
            bool hasPendingAction() const;
            Action getCurrentAction() const;
            juce::Result getLastResult() const;

            // Async operations - schedule work on the background thread.
            // initializeModel also loads or starts a session on completion.
            void setAgentBackend(AgentBackend backend);
            void setFirstQuery(juce::String const& firstQuery);
            void initializeModel(ModelInfo const& info);
            void sendQuery(juce::String const& prompt, nlohmann::json const& mcpToolsContext);
            void startSession();
            void loadSession(juce::File const sessionFile);
            void saveSession(juce::File const sessionFile);
            void cancelQuery();

        private:
            Agent& getCurrentAgent();
            Agent const& getCurrentAgent() const;
            
            Mcp::Dispatcher& mMcpDispatcher;
            std::atomic<AgentBackend> mBackend;
            AgentLocal mAgentLocal;
            AgentRemote mAgentRemote;

            struct PendingAction
            {
                Action action{Action::none};
                std::function<juce::Result()> fn;
            };

            mutable std::mutex mPendingMutex;
            std::condition_variable mPendingCondition;
            std::deque<PendingAction> mPendingActions;

            mutable std::mutex mResultMutex;
            juce::Result mLastResult{juce::Result::ok()};

            std::thread mWorkerThread;
            std::atomic<bool> mShouldExit{false};
            std::atomic<Action> mCurrentAction{Action::none};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackgroundAgent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
