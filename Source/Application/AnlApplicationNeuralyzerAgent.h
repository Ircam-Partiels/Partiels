#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief The interface of the agents that run a model (local or remote).
        //! @details The agent manages the model, the conversation (the history and the tool calls) and the sessions.
        //! The methods that manage the model, the queries and the sessions are called from the same thread (the thread
        //! of the background agent), the other methods are thread-safe.
        class Agent
        {
        public:
            Agent() = default;
            virtual ~Agent() = default;

            //! @brief Initializes the model.
            //! @details The history is cleared.
            //! @param info The model information.
            //! @return The result of the initialization.
            virtual juce::Result initializeModel(ModelInfo const& info) = 0;

            //! @brief Gets if the model information can be applied without initializing the model.
            //! @details The model must be initialized and only the parameters that do not require to initialize the
            //! model can differ (for instance the sampling parameters of a local model).
            //! @param info The model information.
            virtual bool canUpdateModel(ModelInfo const& info) const = 0;

            //! @brief Applies the model information without initializing the model.
            //! @details The history and the context are kept. The model is initialized if the model information
            //! cannot be applied otherwise (see canUpdateModel()).
            //! @param info The model information.
            //! @return The result of the update.
            virtual juce::Result updateModel(ModelInfo const& info) = 0;

            //! @brief Releases the model and clears the history.
            virtual juce::Result resetModel() = 0;

            //! @brief Sends a query and calls the tools requested by the model until the model responds.
            //! @details The history is restored if the query fails.
            //! @param prompt The prompt of the user.
            //! @param useThinkingMode Whether the model reasons before responding.
            //! @return The result of the query.
            virtual juce::Result sendQuery(juce::String const& prompt, bool useThinkingMode) = 0;

            //! @brief Starts a new session with the instructions of the MCP dispatcher.
            virtual juce::Result startSession() = 0;

            //! @brief Loads a session file.
            //! @param sessionFile The session file.
            //! @return The result of the loading.
            virtual juce::Result loadSession(juce::File const& sessionFile) = 0;

            //! @brief Saves the session to a file.
            //! @param sessionFile The session file.
            //! @return The result of the saving.
            virtual juce::Result saveSession(juce::File const& sessionFile) = 0;

            //! @brief Gets the messages of the conversation (thread-safe).
            virtual std::vector<common_chat_msg> getHistory() const = 0;

            //! @brief Gets the raw response during the generation (thread-safe).
            virtual juce::String getTemporaryResponse() const = 0;

            //! @brief Gets the ratio of the context capacity used by the conversation (thread-safe).
            virtual float getContextCapacityUsage() const = 0;

            //! @brief Gets the model information with the parameters resolved during the initialization (thread-safe).
            //! @return The model information or an invalid one if the model is not initialized.
            virtual ModelInfo getModelInfo() const = 0;

            //! @brief Sets whether the initialization of the model or the query should be aborted (thread-safe).
            virtual void setShouldQuit(bool state) = 0;

            //! @brief Gets whether the initialization of the model or the query should be aborted (thread-safe).
            virtual bool shouldQuit() const = 0;

            //! @brief Sets the callback called when the history or the state of the agent changes.
            virtual void setNotifyCallback(std::function<void()> callback) = 0;

            //! @brief Sets the callback called by the MCP tool elicit to ask a question to the user.
            //! @details The callback is called from the thread of the query and must wait for the answer (the
            //! question is cancelled without callback).
            virtual void setElicitCallback(Mcp::DynamicToolMethods::elicit_fn_t callback) = 0;
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
