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

            Chat();
            ~Chat();

            juce::Result initialize(juce::File model, juce::File state);

            std::tuple<juce::Result, juce::String, juce::String> generate(Role const role, juce::String const& prompt, std::atomic<bool> const& shouldQuit);

        private:
            using ModelPtr = std::unique_ptr<llama_model, decltype(&llama_model_free)>;
            using ContextPtr = std::unique_ptr<llama_context, decltype(&llama_free)>;
            using SamplerPtr = std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)>;
            ModelPtr mModel{nullptr, &llama_model_free};
            ContextPtr mContext{nullptr, &llama_free};
            SamplerPtr mSampler{nullptr, &llama_sampler_free};
            llama_vocab const* mVocab = nullptr;
            std::atomic<bool> const* mShouldQuit;

            std::vector<std::string> mMessageData;
            std::vector<llama_chat_message> mMessages;
            std::vector<char> mFormattedMessage;
            long mPrevMessageLength = 0;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace Llama
} // namespace Application

ANALYSE_FILE_END
