#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerMcp.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class AgentRemote
        : public Agent
        {
        public:
            AgentRemote();
            ~AgentRemote() override = default;

            void setFirstQuery(juce::String const& firstQuery) override;
            juce::String getFirstQuery() const override;

            juce::Result initializeModel(ModelInfo const& info) override;
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

            mutable std::mutex mConfigMutex;
            ModelInfo mModelInfo;
            juce::String mFirstQuery;

            mutable std::mutex mSessionMutex;
            History mHistory;
            juce::String mTempResponse;
            juce::String mLastResponseId;

            mutable std::mutex mCallbackMutex;
            std::function<void()> mNotifyCallback;

            std::atomic<bool> mShouldQuit{false};
            std::atomic<float> mContextCapacityUsage{0.0f};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentRemote)
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
