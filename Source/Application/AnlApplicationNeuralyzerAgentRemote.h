#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerRag.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class AgentRemote
        : public Agent
        {
        public:
            AgentRemote(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine);
            ~AgentRemote() override = default;

            juce::Result initializeModel(ModelInfo const& info) override;
            juce::Result resetModel() override;
            juce::Result sendQuery(juce::String const& prompt) override;
            juce::Result startSession() override;
            juce::Result loadSession(juce::File const& sessionFile) override;
            juce::Result saveSession(juce::File const& sessionFile) override;

            std::vector<common_chat_msg> getHistory() const override;
            juce::String getTemporaryResponse() const override;
            float getContextCapacityUsage() const override;
            ModelInfo getModelInfo() const override;

            void setShouldQuit(bool state) override;
            bool shouldQuit() const override;

            void setNotifyCallback(std::function<void()> callback) override;

            static std::set<juce::String> getAvailableModels(juce::URL const& serverUrl);

        private:
            void notifyStateChanged();
            std::vector<common_chat_msg> performInference();
            nlohmann::json readFiles(std::vector<std::string> const& filePaths);

            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine& mRagEngine;
            Mcp::Dispatcher::DynamicToolMethods mMcpMethods;
            std::vector<common_chat_tool> mTools;

            mutable std::mutex mConfigMutex;
            ModelInfo mModelInfo;

            mutable std::mutex mMessagesMutex;
            std::vector<common_chat_msg> mMessages;

            mutable std::mutex mTemporaryMutex;
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
