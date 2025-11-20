#pragma once

#include "../Misc/AnlMisc.h"
#include "llama.h"

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
            juce::Result addContext(juce::String const& content);
            juce::Result loadState(juce::File state);
            std::tuple<juce::Result, juce::String, juce::String> generate(Role const role, juce::String const& prompt);

        private:
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

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Llama
} // namespace Application

ANALYSE_FILE_END
