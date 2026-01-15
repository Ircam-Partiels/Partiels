#pragma once

#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4245 4267 4305 4457 4505)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
#include <common.h>
#include <llama-cpp.h>
#include <mtmd-helper.h>
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
        //! @brief The engine that runs a local model with llama.cpp.
        //! @details The engine loads the model with its chat templates, its multimodal projector and its multi-token
        //! prediction (MTP) model, evaluates the messages in the context memory and generates the responses. The part of
        //! the context memory that matches the messages is reused from one generation to the next. The methods must be
        //! called from the same thread, except the getters documented as thread-safe.
        class EngineLocal
        {
        public:
            //! @brief Creates an engine without model.
            //! @param shouldAbort The flag that aborts the loading of the model and the generation.
            explicit EngineLocal(std::atomic<bool> const& shouldAbort);
            ~EngineLocal();

            //! @brief Loads the model and creates the context.
            //! @details The context size is fitted to the free memory of the devices (the context size of the model
            //! information is used as an upper limit). The multimodal projector and the MTP model are optional.
            //! @param info The model file and the optional parameters of the context and the sampling.
            //! @param projectorFile The multimodal projector file or an empty file.
            //! @param mtpFile The MTP model file or an empty file.
            //! @return The result of the initialization.
            juce::Result initialize(ModelInfo const& info, juce::File const& projectorFile, juce::File const& mtpFile);

            //! @brief Releases the model and the context.
            void reset();

            //! @brief Gets if the model and the context are initialized.
            bool isInitialized() const;

            //! @brief Gets the model information with the parameters resolved during the initialization (thread-safe).
            ModelInfo getModelInfo() const;

            //! @brief Gets the ratio of the context capacity used by the context memory (thread-safe).
            float getContextUsage() const;

            //! @brief Generates the response of the model to the messages.
            //! @details The messages are rendered with the chat templates, the part of the context memory that matches
            //! the prompt is reused and the rest of the prompt is evaluated. A media message whose file is no longer
            //! available is replaced by a text.
            //! @param messages The messages of the conversation.
            //! @param tools The tools that the model can call.
            //! @param useThinkingMode Whether the model reasons before responding.
            //! @param onGenerationStarted Called when the prompt has been evaluated and the generation starts.
            //! @param onResponseChanged Called with the raw response during the generation.
            //! @return The parsed response, or an empty message if the generation has been aborted.
            //! @throw juce::String The error message if the generation fails.
            common_chat_msg generate(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode, std::function<void()> onGenerationStarted, std::function<void(std::string const&)> onResponseChanged);

            //! @brief Clears the context memory.
            void clearContext();

            //! @brief Gets the number of tokens in the context memory.
            size_t getContextSize() const;

            //! @brief Gets the capacity of the context memory in tokens.
            size_t getContextCapacity() const;

            //! @brief Gets the number of tokens of the system prompt (the first message and the tools).
            //! @param messages The messages of the conversation (only the first message is used).
            //! @param tools The tools that the model can call.
            //! @param useThinkingMode Whether the model reasons before responding (the template may depend on it).
            //! @return The number of tokens or 0 if the prompt cannot be rendered.
            size_t getSystemPromptSize(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode) const;

            //! @brief Gets if the model supports the media (with a multimodal projector).
            bool supportsMedia() const;

            //! @brief Checks that a media file can be loaded by the multimodal projector.
            //! @param file The image or audio file.
            //! @return The result of the check.
            juce::Result checkMedia(juce::File const& file) const;

            //! @brief Creates a message that injects a media file in the conversation.
            //! @details The media file is reloaded each time the message is evaluated.
            //! @param file The image or audio file.
            //! @return The media message.
            static common_chat_msg createMediaMessage(juce::File const& file);

            //! @brief Gets the version of the state files (that must be saved with the state files).
            static int getStateVersion();

            //! @brief Saves the context memory to a state file.
            //! @details A context memory that contains media cannot be restored, so the state file is deleted.
            //! @param file The state file.
            //! @return The result of the saving.
            juce::Result saveState(juce::File const& file) const;

            //! @brief Restores the context memory from a state file.
            //! @details The context memory is cleared if the state cannot be restored.
            //! @param file The state file.
            //! @return The result of the restoring.
            juce::Result loadState(juce::File const& file);

            //! @brief Gets the error message when the context is full.
            static juce::String getContextFullMessage();

        private:
            //! @brief A part of the content evaluated in the context memory: text tokens or a media.
            struct ContextChunk
            {
                llama_tokens tokens;       //!< The tokens of a text chunk
                std::string mediaId;       //!< The identifier of a media chunk (empty for a text chunk)
                llama_pos numPositions{0}; //!< The number of positions used in the context
                size_t numTokens{0};       //!< The number of tokens used in the context memory
            };

            //! @brief Evaluates the part of the prompt that is not in the context memory.
            void evaluatePrompt(std::vector<common_chat_msg> const& messages, common_chat_params const& chatParams);

            //! @brief Gets the position that follows the content of the context memory.
            llama_pos getContextPosition() const;

            //! @brief Appends tokens decoded in the context memory to the tracked content.
            void appendContextTokens(llama_token const* tokens, size_t numTokens);

            //! @brief Truncates the tracked content to a position.
            //! @return False if the position is inside a media.
            bool truncateContextChunks(llama_pos position);

            //! @brief Saves the partial states of the context memory that cannot be rolled back.
            void createContextCheckpoint(llama_pos position);

            //! @brief Removes the content of the context memory after a position.
            //! @return The position actually kept (before the position if the memory cannot be removed partially).
            llama_pos rollbackContextMemory(llama_pos position);

            //! @brief Clears the context memory and the tracked content.
            void clearContextMemory();

            //! @brief Clears the speculative context memory.
            void resetSpeculativeMemory();

            //! @brief Updates the ratio of the context capacity used by the context memory.
            void updateContextUsage();

            std::atomic<bool> const& mShouldAbort;
            common_init_result_ptr mInitResult;
            common_speculative_init_result_ptr mMtpInitResult;
            common_speculative_ptr mSpeculative;
            std::atomic<bool> mSpeculativeSynchronized{false};
            common_chat_templates_ptr mChatTemplates;
            common_params_sampling mSamplingParams;
            mtmd::context_ptr mMtmdContext;

            std::vector<ContextChunk> mContextChunks;
            common_prompt_checkpoint mContextCheckpoint;
            std::atomic<float> mContextUsage{0.0f};

            mutable std::mutex mModelInfoMutex;
            ModelInfo mModelInfo;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineLocal)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
