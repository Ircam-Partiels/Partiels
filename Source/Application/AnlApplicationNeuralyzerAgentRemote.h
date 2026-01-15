#pragma once

#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerEngineRemote.h"
#include "AnlApplicationNeuralyzerRag.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief The agent that runs a model on a remote server.
        //! @details The agent manages the conversation (the history, the tool calls and the sessions) and uses the remote
        //! engine to generate the responses.
        class AgentRemote
        : public Agent
        {
        public:
            //! @brief Creates an agent without model.
            //! @param mcpDispatcher The dispatcher of the MCP tools.
            //! @param ragEngine The engine that searches the documentation.
            AgentRemote(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine);
            ~AgentRemote() override = default;

            //! @brief Checks that the model is available on the server.
            //! @details The history is cleared.
            juce::Result initializeModel(ModelInfo const& info) override;

            //! @brief Releases the model and clears the history.
            juce::Result resetModel() override;

            //! @brief Sends a query and calls the tools requested by the model until the model responds.
            //! @details The history is restored if the query fails.
            juce::Result sendQuery(juce::String const& prompt, bool useThinkingMode) override;

            //! @brief Starts a new session with the instructions of the MCP dispatcher.
            juce::Result startSession() override;

            //! @brief Loads a session file.
            juce::Result loadSession(juce::File const& sessionFile) override;

            //! @brief Saves the session to a file.
            juce::Result saveSession(juce::File const& sessionFile) override;

            //! @brief Gets the messages of the conversation (thread-safe).
            std::vector<common_chat_msg> getHistory() const override;

            //! @brief Gets the reasoning and the content received during the generation (thread-safe).
            juce::String getTemporaryResponse() const override;

            //! @brief Gets the ratio of the context capacity used by the last generation (thread-safe).
            float getContextCapacityUsage() const override;

            //! @brief Gets the model information with the context size retrieved during the initialization (thread-safe).
            ModelInfo getModelInfo() const override;

            //! @brief Sets whether the initialization of the model or the query should be aborted (thread-safe).
            void setShouldQuit(bool state) override;

            //! @brief Gets whether the initialization of the model or the query should be aborted (thread-safe).
            bool shouldQuit() const override;

            //! @brief Sets the callback called when the history or the state of the agent changes.
            void setNotifyCallback(std::function<void()> callback) override;

            //! @brief Gets the models available on a server.
            //! @param serverUrl The server URL.
            //! @return The identifiers of the models and whether they support images.
            static std::set<std::pair<juce::String, bool>> getAvailableModels(juce::URL const& serverUrl);

        private:
            //! @brief Calls the notify callback.
            void notifyStateChanged();

            //! @brief Generates the response of the model to the history.
            //! @throw juce::String The error message if the generation fails.
            common_chat_msg performInference(bool useThinkingMode);

            //! @brief Reads the files requested by the MCP tool (the media files are attached to the tool response).
            nlohmann::json readFiles(std::vector<std::string> const& filePaths);

            std::atomic<bool> mShouldQuit{false};
            Mcp::Dispatcher& mMcpDispatcher;
            Rag::Engine& mRagEngine;
            Mcp::Dispatcher::DynamicToolMethods mMcpMethods;
            std::vector<common_chat_tool> mTools;
            EngineRemote mEngine{mShouldQuit};

            mutable std::mutex mMessagesMutex;
            std::vector<common_chat_msg> mMessages;

            mutable std::mutex mTemporaryMutex;
            std::string mTempResponse;

            mutable std::mutex mCallbackMutex;
            std::function<void()> mNotifyCallback;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AgentRemote)
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
