#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"
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
        class Agent
        {
        public:
            // clang-format off
            enum class MessageType
            {
                  assistant
                , user
                , system
                , tool
            };
            // clang-format on
            using History = std::vector<std::pair<MessageType, juce::String>>;

            static void initialize();
            static void release();

            Agent(Mcp::Dispatcher& mcpDispatcher);
            ~Agent();

            Mcp::Dispatcher& getMcpDispatcher();

            void setInstructions(juce::String const& instructions, juce::String const& firstQuery);
            juce::String getInstructions() const;
            juce::String getFirstQuery() const;
            void setSessionFiles(juce::File const& contextFile, juce::File const& messageFile);
            std::pair<juce::File, juce::File> getSessionFiles() const;

            juce::Result initializeModel(ModelInfo const& info);
            juce::Result sendQuery(juce::String const& prompt);
            juce::Result startSession();
            juce::Result loadSession();
            juce::Result saveSession();

            History getHistory() const;
            juce::String getTemporaryResponse() const;
            float getContextCapacityUsage() const;
            ModelInfo getModelInfo() const;

            void setShouldQuit(bool state);
            bool shouldQuit() const;

            void setNotifyCallback(std::function<void()> callback);

        private:
            void notifyStateChanged();
            void updateContextMemoryUsage();
            juce::Result manageContentMemory(size_t minNumRequiredTokens);
            juce::Result clearContextMemory();
            juce::Result loadContextFromFile();
            juce::Result saveContextToFile();

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
            juce::String mInstructions;
            juce::String mFirstQuery;

            mutable std::mutex mSessionMutex;
            juce::File mSessionContextFile;
            juce::File mSessionMessageFile;

            mutable std::mutex mNotifyMutex;
            std::function<void()> mNotifyCallback;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Agent)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
