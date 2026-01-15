#pragma once

#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <llama-cpp.h>

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Mcp
        {
            static auto constexpr jsonrpc = "2.0";
            nlohmann::json createError(juce::String const& what);

            class ExecutionError
            : public std::runtime_error
            {
                using std::runtime_error::runtime_error;
            };

            class Connection;

            class Dispatcher
            {
            public:
                Dispatcher() = default;
                ~Dispatcher() = default;

                void setContext(nlohmann::json const& params);

                std::string getUuid();
                std::string getInstructions();
                std::vector<std::pair<std::string, std::string>> getResources();
                std::vector<common_chat_tool> getToolList();
                std::pair<bool, std::vector<std::string>> callTools(std::vector<common_chat_tool_call> const& toolCalls);

            private:
                nlohmann::json callMethod(nlohmann::json const& params);

                nlohmann::json mContext;
                friend class Connection;
            };

            class Connection
            : public juce::InterprocessConnection
            {
            public:
                Connection(Dispatcher& dispatcher);
                ~Connection() override;

                // juce::InterprocessConnection
                void connectionMade() override;
                void connectionLost() override;
                void messageReceived(juce::MemoryBlock const& message) override;

            private:
                Dispatcher& mDispatcher;
            };

            class Server
            : public juce::InterprocessConnectionServer
            {
            public:
                Server(Accessor& accessor, Dispatcher& dispatcher);
                ~Server() override;

                static bool setClaudeApplicationEnabled(bool enabled);
                static bool setCopilotApplicationEnabled(bool enabled);

            protected:
                // juce::InterprocessConnectionServer
                juce::InterprocessConnection* createConnectionObject() override;

            private:
                static juce::String getPartielsExecutablePath();

                Accessor& mAccessor;
                Dispatcher& mDispatcher;
                Accessor::Listener mListener{typeid(*this).name()};
                std::unique_ptr<juce::InterprocessConnection> mConnections;
            };

            namespace Host
            {
                static auto constexpr defaultPort = 3939;
                static auto constexpr defaultArg = "--mcp-host";
                bool run(int port);
            } // namespace Host
        } // namespace Mcp
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
