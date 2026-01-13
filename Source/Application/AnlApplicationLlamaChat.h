#pragma once

#include "../Misc/AnlMisc.h"
#include <llama.h>

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Llama
    {
        class Chat
        {
        public:
            // clang-format off
            enum class Role
            {
                  assistant
                , system
                , user
            };
            // clang-format on

            Chat(std::atomic<bool> const& shouldQuit);
            ~Chat();

            juce::Result initialize(juce::File model);
            juce::Result loadState(juce::File state);
            juce::Result saveState(juce::File state);
            juce::Result injectContext(juce::String const& content);
            juce::Result addSystemMessage(juce::String const& instruction);
            std::tuple<juce::Result, std::string> sendUserQuery(juce::String const& prompt, std::function<bool(std::string const&)> progressCallback);
            juce::String getTemporaryResponse() const;
            void clearTemporaryResponse();

        private:
            long addMessage(Role const role, std::string const& content);

            using ModelPtr = std::unique_ptr<llama_model, decltype(&llama_model_free)>;
            using ContextPtr = std::unique_ptr<llama_context, decltype(&llama_free)>;
            using SamplerPtr = std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)>;
            std::atomic<bool> const& mShouldQuit;
            ModelPtr mModel{nullptr, &llama_model_free};
            ContextPtr mContext{nullptr, &llama_free};
            SamplerPtr mSampler{nullptr, &llama_sampler_free};

            std::vector<std::string> mMessageData;
            std::vector<llama_chat_message> mMessages;
            std::vector<char> mFormattedMessage;
            long mPrevMessageLength = 0;
            mutable std::mutex mTempResponseMutex;
            std::string mTempResponse;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Llama
} // namespace Application

ANALYSE_FILE_END
