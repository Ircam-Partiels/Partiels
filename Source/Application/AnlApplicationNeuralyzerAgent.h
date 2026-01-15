#pragma once

#include "AnlApplicationNeuralyzerMcp.h"
#include "AnlApplicationNeuralyzerModel.h"

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
                , warning
            };
            // clang-format on

            using History = std::vector<std::pair<MessageType, juce::String>>;

            Agent() = default;
            virtual ~Agent() = default;

            virtual void setFirstQuery(juce::String const& firstQuery) = 0;
            virtual juce::String getFirstQuery() const = 0;

            virtual juce::Result initializeModel(ModelInfo const& info) = 0;
            virtual juce::Result resetModel() = 0;
            virtual juce::Result sendQuery(juce::String const& prompt) = 0;
            virtual juce::Result startSession() = 0;
            virtual juce::Result loadSession(juce::File const& sessionFile) = 0;
            virtual juce::Result saveSession(juce::File const& sessionFile) = 0;

            virtual History getHistory() const = 0;
            virtual juce::String getTemporaryResponse() const = 0;
            virtual float getContextCapacityUsage() const = 0;
            virtual ModelInfo getModelInfo() const = 0;

            virtual void setShouldQuit(bool state) = 0;
            virtual bool shouldQuit() const = 0;

            virtual void setNotifyCallback(std::function<void()> callback) = 0;

            static std::string getErrorInstuction(size_t const maxErrors, size_t const numErrors);
            static std::string callTools(Mcp::Dispatcher& mcpDispatcher, std::vector<common_chat_tool_call> const& toolCalls, size_t const maxErrors, size_t& numErrors);

        protected:
            static auto constexpr kNumInitMessages = 3_z;
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
