#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerEngineLocal.h"
#include "AnlApplicationNeuralyzerRag.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief The agent that runs a local model.
        //! @details The agent manages the conversation (the history, the tool calls, the summary and the sessions) and
        //! uses the local engine to generate the responses. It also manages the model files (the discovery and the
        //! download of the models).
        class AgentLocal
        : public Agent
        {
        public:
            //! @brief Creates an agent without model.
            //! @param mcpDispatcher The dispatcher of the MCP tools.
            //! @param ragEngine The engine that searches the documentation.
            AgentLocal(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine);
            ~AgentLocal() override = default;

            //! @brief Gets the dispatcher of the MCP tools.
            Mcp::Dispatcher& getMcpDispatcher();

            //! @brief Loads the model with its multimodal projector and its MTP model (found next to the model file).
            //! @details The history is cleared.
            juce::Result initializeModel(ModelInfo const& info) override;

            //! @brief Releases the model and clears the history.
            juce::Result resetModel() override;

            //! @brief Sends a query and calls the tools requested by the model until the model responds.
            //! @details The session is summarized beforehand if the context is almost full and if the summary would
            //! free a significant part of the context. The history is restored if the query fails.
            juce::Result sendQuery(juce::String const& prompt, bool useThinkingMode) override;

            //! @brief Starts a new session with the instructions of the MCP dispatcher.
            juce::Result startSession() override;

            //! @brief Loads a session file and the state of the context memory saved next to it (if valid).
            juce::Result loadSession(juce::File const& sessionFile) override;

            //! @brief Saves the session to a file and the state of the context memory next to it.
            juce::Result saveSession(juce::File const& sessionFile) override;

            //! @brief Adds a media file (image or audio) to the history.
            //! @param file The media file.
            //! @return The result of the operation.
            juce::Result addMedia(juce::File const& file);

            //! @brief Gets the messages of the conversation (thread-safe).
            std::vector<common_chat_msg> getHistory() const override;

            //! @brief Gets the raw response during the generation (thread-safe).
            juce::String getTemporaryResponse() const override;

            //! @brief Gets the ratio of the context capacity used by the context memory (thread-safe).
            float getContextCapacityUsage() const override;

            //! @brief Gets the model information with the parameters resolved during the initialization (thread-safe).
            ModelInfo getModelInfo() const override;

            //! @brief Sets whether the loading of the model or the query should be aborted (thread-safe).
            void setShouldQuit(bool state) override;

            //! @brief Gets whether the loading of the model or the query should be aborted (thread-safe).
            bool shouldQuit() const override;

            //! @brief Sets the callback called when the history or the state of the agent changes.
            void setNotifyCallback(std::function<void()> callback) override;

            //! @brief Gets the directory where the default models are downloaded.
            static juce::File getDefaultModelDirectory();

            //! @brief Gets the model files available in the Partiels and Hugging Face directories.
            static std::set<juce::File> getAvailableModels();

            //! @brief Gets the multimodal projector file next to a model file (or an empty file).
            static juce::File getProjectorForModel(juce::File const& modelFile);

            //! @brief Gets the MTP model file next to a model file (or an empty file).
            static juce::File getMtpForModel(juce::File const& modelFile);

            //! @brief The files and the URLs of a model, its multimodal projector and its optional MTP model.
            struct ModelBundle
            {
                juce::URL modelUrl;       //!< The URL of the model
                juce::File modelFile;     //!< The model file
                juce::URL projectorUrl;   //!< The URL of the multimodal projector
                juce::File projectorFile; //!< The multimodal projector file
                juce::URL mtpUrl;         //!< The URL of the MTP model (optional)
                juce::File mtpFile;       //!< The MTP model file (optional)
            };

            //! @brief Gets the bundles of the models that can be downloaded.
            static std::vector<ModelBundle> getDefaultModelBundles();

            //! @brief Gets the bundle of the model downloaded by default.
            static ModelBundle getDefaultModelBundle();

            //! @brief Downloads the default model if no model is available.
            static void downloadDefaultModelIfNecessary();

            //! @brief Downloads the files of a model bundle.
            //! @param bundle The model bundle.
            //! @param warnIfFailed Whether a message is shown if the download fails.
            static void downloadModelBundle(ModelBundle const& bundle, bool warnIfFailed);

        private:
            //! @brief Calls the notify callback.
            void notifyStateChanged();

            //! @brief Generates the response of the model to the history.
            //! @throw juce::String The error message if the generation fails.
            common_chat_msg performInference(bool useThinkingMode);

            //! @brief Replaces the history by a summary of the conversation.
            //! @details The history is restored if the summary fails.
            juce::Result summarizeSession(bool useThinkingMode);

            //! @brief Reads the files requested by the MCP tool (the media files are added to the history).
            nlohmann::json readFiles(std::vector<std::string> const& filePaths);

            std::atomic<bool> mShouldQuit{false};
            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine& mRagEngine;
            Mcp::Dispatcher::DynamicToolMethods mMcpMethods;
            std::vector<common_chat_tool> mTools;
            EngineLocal mEngine{mShouldQuit};

            mutable std::mutex mMessagesMutex;
            std::vector<common_chat_msg> mMessages;

            mutable std::mutex mTemporaryMutex;
            std::string mTempResponse;

            mutable std::mutex mNotifyMutex;
            std::function<void()> mNotifyCallback;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentLocal)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
