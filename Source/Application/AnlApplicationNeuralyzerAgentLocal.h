#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerRag.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
#include <common.h>
#include <mtmd.h>
#include <sampling.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <llama-cpp.h>

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class AgentLocal
        : public Agent
        {
        public:
            AgentLocal(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine);
            ~AgentLocal() override;

            Mcp::Dispatcher& getMcpDispatcher();

            juce::Result initializeModel(ModelInfo const& info) override;
            juce::Result resetModel() override;
            juce::Result sendQuery(juce::String const& prompt) override;
            juce::Result startSession() override;
            juce::Result loadSession(juce::File const& sessionFile) override;
            juce::Result saveSession(juce::File const& sessionFile) override;
            juce::Result addMedia(juce::File const& file);

            std::vector<common_chat_msg> getHistory() const override;
            juce::String getTemporaryResponse() const override;
            float getContextCapacityUsage() const override;
            ModelInfo getModelInfo() const override;

            void setShouldQuit(bool state) override;
            bool shouldQuit() const override;

            void setNotifyCallback(std::function<void()> callback) override;

            static juce::File getDefaultModelDirectory();
            static juce::File getDefaultProjectorDirectory();
            static std::set<juce::File> getAvailableModels();
            static std::set<juce::File> getAvailableProjectors();

            struct ModelBundle
            {
                juce::String name;
                juce::URL modelUrl;
                juce::File modelFile;
                juce::URL projectorUrl;
                juce::File projectorFile;
            };
            static std::vector<ModelBundle> getDefaultModelBundles();
            static ModelBundle getDefaultModelBundle();
            static void downloadDefaultModelIfNecessary();
            static void downloadModelBundle(ModelBundle const& bundle, bool warnIfFailed);

        private:
            void notifyStateChanged();
            std::vector<common_chat_msg> performInference();
            juce::Result summarizeSession();

            void updateContextMemoryUsage();

            nlohmann::json readFiles(std::vector<std::string> const& filePaths);
            std::atomic<bool> mShouldQuit{false};
            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine& mRagEngine;
            Mcp::Dispatcher::DynamicToolMethods mMcpMethods;
            std::vector<common_chat_tool> mTools;
            common_init_result_ptr mInitResult;
            common_chat_templates_ptr mChatTemplates;
            common_params_sampling mSamplingParams;
            mtmd::context_ptr mMtmdContext;

            mutable std::mutex mMessagesMutex;
            std::vector<common_chat_msg> mMessages;
            size_t mPastMessagesPosition{0_z};

            mutable std::mutex mTemporaryMutex;
            std::string mTempResponse;

            mutable std::mutex mModelInfoMutex;
            ModelInfo mModelInfo;
            std::atomic<float> mContextMemoryUsage{0.0f};

            mutable std::mutex mInstructionsMutex;

            mutable std::mutex mNotifyMutex;
            std::function<void()> mNotifyCallback;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentLocal)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
