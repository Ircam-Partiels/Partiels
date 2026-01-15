#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
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

            juce::Result initialize(ModelInfo const& info, juce::String const& instructions);
            std::tuple<juce::Result, std::string> sendUserQuery(juce::String const& prompt, bool allowTools);
            juce::String getTemporaryResponse() const;
            void clearTemporaryResponse();
            float getContextCapacityUsage() const;
            ModelInfo getModelInfo() const;

            juce::Result loadState(juce::File state);
            juce::Result saveState(juce::File state);
            void resetState();

            void setShouldQuit(bool state);
            bool shouldQuit() const;

        private:
            juce::Result ensureContextSpace(size_t minNumRequiredTokens);
            void updateContextCapacity();
            std::tuple<juce::Result, std::string, common_chat_params> performQuery(std::string const& role, std::string query, bool allowTools);

            std::atomic<bool> mShouldQuit;
            Mcp::Dispatcher& mMcpDispatcher;
            common_init_result_ptr mInitResult;
            common_chat_templates_ptr mChatTemplates;
            common_chat_templates_inputs mChatInputs;    // Cached inputs for chat template application and messages
            std::vector<llama_pos> mMessageEndPositions; // KV cache position where each message ends
            mutable std::mutex mTempResponseMutex;
            std::string mTempResponse;
            ModelInfo mModelInfo;
            std::atomic<float> mContextCapacityUsage{0.0f};
            std::mutex mCallMutex;
            juce::String mInstructions;

            static auto constexpr userRole = "user";

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Agent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
