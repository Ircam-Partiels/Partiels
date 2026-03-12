#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor")
#include <chat.h>
#include <common.h>
#include <sampling.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <llama-cpp.h>

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

            juce::Result initialize(ModelInfo info);
            std::tuple<juce::Result, std::string> sendUserQuery(juce::String const& prompt, bool allowTools);
            juce::String getTemporaryResponse() const;
            void clearTemporaryResponse();

            juce::Result loadState(juce::File state);
            juce::Result saveState(juce::File state);
            void resetState();

            void setShouldQuit(bool state);
            bool shouldQuit() const;

        private:
            void pruneMessagesFromContext(juce::Range<int32_t> messageRange);
            std::tuple<juce::Result, std::string, common_chat_params> performQuery(std::string const& role, std::string query, bool allowTools);
            juce::Result manageContextSize();

            std::atomic<bool> mShouldQuit;
            Mcp::Dispatcher& mNeuralyzerMcpDispatcher;
            common_init_result_ptr mInitResult;
            common_chat_templates_ptr mChatTemplates;
            common_chat_templates_inputs mChatInputs;    // Cached inputs for chat template application and messages
            std::vector<llama_pos> mMessageEndPositions; // KV cache position where each message ends
            mutable std::mutex mTempResponseMutex;
            std::string mTempResponse;
            std::mutex mCallMutex;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Agent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
