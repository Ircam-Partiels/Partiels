#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerRag.h"
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
        class AgentLocal
        : public Agent
        {
        public:
            static void initialize();
            static void release();

            AgentLocal(Mcp::Dispatcher& mcpDispatcher);
            ~AgentLocal() override;

            Mcp::Dispatcher& getMcpDispatcher();

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
            void updateContextMemoryUsage();
            juce::Result manageContentMemory(size_t minNumRequiredTokens);
            juce::Result clearContextMemory();

            juce::Result performQuery(juce::String const& query, bool allowTools);
            std::tuple<juce::Result, std::string, common_chat_params> performMessage(std::string const& role, std::string query, bool allowTools);

            std::atomic<bool> mShouldQuit{false};
            Mcp::Dispatcher& mMcpDispatcher;
            common_init_result_ptr mInitResult;
            common_chat_templates_ptr mChatTemplates;

            mutable std::mutex mHistoryMutex;
            common_chat_templates_inputs mChatInputs;

            mutable std::mutex mTemporaryMutex;
            std::string mTempResponse;

            mutable std::mutex mModelInfoMutex;
            ModelInfo mModelInfo;
            std::atomic<float> mContextMemoryUsage{0.0f};

            mutable std::mutex mInstructionsMutex;
            juce::String mFirstQuery;

            mutable std::mutex mNotifyMutex;
            std::function<void()> mNotifyCallback;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentLocal)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
