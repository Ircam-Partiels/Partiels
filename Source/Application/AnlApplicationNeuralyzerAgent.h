#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant")
#include <chat.h>
#include <common.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <llama.h>

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class Agent
        {
        public:
            static void initialize();
            static void release();

            Agent(Mcp::Dispatcher& mcpDispatcher);
            ~Agent();

            Mcp::Dispatcher& getMcpDispatcher();

            juce::Result initialize(ModelInfo info, juce::String const& instruction);
            juce::Result loadState(juce::File state);
            juce::Result saveState(juce::File state);
            std::tuple<juce::Result, std::string> sendUserQuery(juce::String const& prompt);
            juce::String getTemporaryResponse() const;
            void clearTemporaryResponse();

            void setShouldQuit(bool state);
            bool shouldQuit() const;

        private:
            void pruneMessagesFromContext(juce::Range<llama_pos> messageRange);
            juce::Result sendSystemMessage(std::string instruction);
            std::tuple<juce::Result, std::string, common_chat_params> performUserQuery(std::string query, bool allowTools);
            juce::Result manageContextSize();

            using ModelPtr = std::unique_ptr<llama_model, decltype(&llama_model_free)>;
            using ContextPtr = std::unique_ptr<llama_context, decltype(&llama_free)>;
            using SamplerPtr = std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)>;
            std::atomic<bool> mShouldQuit;
            Mcp::Dispatcher& mNeuralyzerMcpDispatcher;
            ModelPtr mModel{nullptr, &llama_model_free};
            ContextPtr mContext{nullptr, &llama_free};
            SamplerPtr mSampler{nullptr, &llama_sampler_free};

            common_chat_templates_ptr mChatTemplates;
            common_chat_templates_inputs mChatInputs; // Cached inputs for chat template application and messages
            size_t mPreviousPromptSize = 0;
            std::vector<llama_pos> mMessageEndPositions; // KV cache position where each message ends

            mutable std::mutex mTempResponseMutex;
            std::string mTempResponse;
            std::mutex mCallMutex;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Agent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
