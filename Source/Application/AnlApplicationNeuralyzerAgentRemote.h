#pragma once

#include "AnlApplicationNeuralyzerAgent.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class AgentRemote
        : public Agent
        {
        public:
            explicit AgentRemote(Mcp::Dispatcher& mcpDispatcher);
            ~AgentRemote() override = default;

            void setFirstQuery(juce::String const& firstQuery) override;
            juce::String getFirstQuery() const override;

            juce::Result initializeModel(ModelInfo const& info) override;
            juce::Result resetModel() override;
            juce::Result sendQuery(juce::String const& prompt) override;
            juce::Result startSession() override;
            juce::Result loadSession(juce::File const& sessionFile) override;
            juce::Result saveSession(juce::File const& sessionFile) override;

            History getHistory() const override;
            juce::String getTemporaryResponse() const override;
            float getContextCapacityUsage() const override;
            ModelInfo getModelInfo() const override;

            void setShouldQuit(bool state) override;
            bool shouldQuit() const override;

            void setNotifyCallback(std::function<void()> callback) override;

        private:
            void notifyStateChanged();
            juce::Result performQuery(juce::String const& prompt, bool allowTools);
            std::tuple<juce::Result, std::vector<common_chat_tool_call>> performMessage(MessageType const& role, juce::String const& prompt, bool allowTools);

            Mcp::Dispatcher& mMcpDispatcher;
            mutable std::mutex mConfigMutex;
            ModelInfo mModelInfo;
            juce::String mFirstQuery;

            mutable std::mutex mSessionMutex;
            History mHistory;
            juce::String mTempResponse;

            mutable std::mutex mCallbackMutex;
            std::function<void()> mNotifyCallback;

            std::atomic<bool> mShouldQuit{false};
            std::atomic<float> mContextCapacityUsage{0.0f};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentRemote)
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
