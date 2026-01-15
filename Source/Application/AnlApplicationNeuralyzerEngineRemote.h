#pragma once

#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4245 4267 4305 4457 4505)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief The engine that runs a model on a remote server with the OpenAI-compatible API.
        //! @details The engine checks that the model is available on the server, retrieves its context size (with the
        //! llama.cpp and LM Studio endpoints) and generates the responses with streamed chat completions. The server URL
        //! can contain a base path and can end with the version of the API (e.g. http://localhost:1234/v1). The methods
        //! must be called from the same thread, except the getters documented as thread-safe.
        class EngineRemote
        {
        public:
            //! @brief Creates an engine without model.
            //! @param shouldAbort The flag that aborts the requests (including the pending ones).
            explicit EngineRemote(std::atomic<bool> const& shouldAbort);
            ~EngineRemote() = default;

            //! @brief Checks that the model is available on the server and retrieves its context size.
            //! @param info The server URL and the model identifier.
            //! @return The result of the initialization.
            juce::Result initialize(ModelInfo const& info);

            //! @brief Releases the model.
            void reset();

            //! @brief Gets if the model is initialized (thread-safe).
            bool isInitialized() const;

            //! @brief Gets the model information with the context size retrieved during the initialization (thread-safe).
            ModelInfo getModelInfo() const;

            //! @brief Gets the ratio of the context capacity used by the last generation (thread-safe).
            //! @details The ratio remains 0 if the server does not report the context size or the token usage.
            float getContextUsage() const;

            //! @brief Generates the response of the model to the messages.
            //! @details The response is streamed so the generation can be followed and aborted. The media content parts
            //! (image_url and input_audio) of the messages are sent with the OpenAI-compatible formats.
            //! @param messages The messages of the conversation.
            //! @param tools The tools that the model can call.
            //! @param useThinkingMode Whether the model reasons before responding.
            //! @param onGenerationStarted Called when the server has evaluated the prompt and the generation starts.
            //! @param onResponseChanged Called with the reasoning and the content received during the generation.
            //! @return The response, or an empty message if the generation has been aborted.
            //! @throw juce::String The error message if the generation fails.
            common_chat_msg generate(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode, std::function<void()> onGenerationStarted, std::function<void(std::string const&)> onResponseChanged);

            //! @brief Gets the models available on a server.
            //! @param serverUrl The server URL.
            //! @return The identifiers of the models and whether they support images.
            static std::set<std::pair<juce::String, bool>> getAvailableModels(juce::URL const& serverUrl);

        private:
            std::atomic<bool> const& mShouldAbort;
            std::atomic<float> mContextUsage{0.0f};

            mutable std::mutex mModelInfoMutex;
            ModelInfo mModelInfo;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineRemote)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
