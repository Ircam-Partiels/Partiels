#pragma once

#include "AnlApplicationNeuralyzerAgent.h"

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
                , initializeModel
                , sendQuery
                , startSession
                , loadSession
                , saveSession
                , cancelQuery
            };
            // clang-format on

            BackgroundAgent(Mcp::Dispatcher& mcpDispatcher, juce::String const& instructions, juce::String const& firstQuery);
            ~BackgroundAgent();

            // Getters that are safe to call from any thread (pass through to Agent)
            Agent::History getHistory() const;
            juce::String getTemporaryResponse() const;
            float getContextCapacityUsage() const;
            ModelInfo getModelInfo() const;

            // Async operations - schedule work on the background thread.
            // initializeModel also loads or starts a session on completion.
            void initializeModel(ModelInfo const& info);
            void sendQuery(juce::String const& prompt);
            void startSession();
            void loadSession(juce::File const contextFile, juce::File const messageFile);
            void saveSession(juce::File const contextFile, juce::File const messageFile);
            void cancelQuery();

            // State inspection
            bool hasPendingAction() const;
            Action getCurrentAction() const;
            juce::Result getLastResult() const;

        private:
            Agent mAgent;

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
