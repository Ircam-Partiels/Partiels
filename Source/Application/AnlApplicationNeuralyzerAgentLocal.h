#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerRag.h"
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4245 4267 4305 4457 4505)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
#include <common.h>
#include <llama-cpp.h>
#include <mtmd.h>
#include <sampling.h>
#include <speculative.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

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
            juce::Result sendQuery(juce::String const& prompt, bool useThinkingMode) override;
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
            static std::set<juce::File> getAvailableModels();
            static juce::File getProjectorForModel(juce::File const& modelFile);
            static juce::File getMtpForModel(juce::File const& modelFile);

            struct ModelBundle
            {
                juce::URL modelUrl;
                juce::File modelFile;
                juce::URL projectorUrl;
                juce::File projectorFile;
                juce::URL mtpUrl;
                juce::File mtpFile;
            };
            static std::vector<ModelBundle> getDefaultModelBundles();
            static ModelBundle getDefaultModelBundle();
            static void downloadDefaultModelIfNecessary();
            static void downloadModelBundle(ModelBundle const& bundle, bool warnIfFailed);

        private:
            // A chunk of the content evaluated in the context: text tokens or a media
            struct ContextChunk
            {
                llama_tokens tokens;       // The tokens of a text chunk
                std::string mediaId;       // The identifier of a media chunk (empty for a text chunk)
                llama_pos numPositions{0}; // The number of positions used in the context
            };

            void notifyStateChanged();
            std::vector<common_chat_msg> performInference(bool useThinkingMode);
            void evaluatePrompt(std::vector<common_chat_msg> const& messages, common_chat_params const& chatParams, std::function<bool(void)> callback);
            juce::Result summarizeSession(bool useThinkingMode);
            llama_pos getSystemPromptSize(bool useThinkingMode) const;

            void updateContextMemoryUsage();
            void resetSpeculativeMemory();

            llama_pos getContextPosition() const;
            void appendContextTokens(llama_token const* tokens, size_t numTokens);
            bool truncateContextChunks(llama_pos position);
            void createContextCheckpoint(llama_pos position);
            llama_pos rollbackContextMemory(llama_pos position);
            void clearContextMemory();

            nlohmann::json readFiles(std::vector<std::string> const& filePaths);
            std::atomic<bool> mShouldQuit{false};
            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine& mRagEngine;
            Mcp::Dispatcher::DynamicToolMethods mMcpMethods;
            std::vector<common_chat_tool> mTools;
            common_init_result_ptr mInitResult;
            common_speculative_init_result_ptr mMtpInitResult;
            common_speculative_ptr mSpeculative;
            std::atomic<bool> mSpeculativeSynchronized{false};
            common_chat_templates_ptr mChatTemplates;
            common_params_sampling mSamplingParams;
            mtmd::context_ptr mMtmdContext;

            mutable std::mutex mMessagesMutex;
            std::vector<common_chat_msg> mMessages;

            std::vector<ContextChunk> mContextChunks;
            common_prompt_checkpoint mContextCheckpoint;

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
