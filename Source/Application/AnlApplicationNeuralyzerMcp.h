#pragma once

#include "AnlApplicationNeuralyzerMcpDispatcher.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Mcp
        {
            class Connection
            : public juce::InterprocessConnection
            {
            public:
                //! @brief Creates a connection with an external client
                //! @param dispatcher The dispatcher of the MCP methods
                //! @param methods The methods of the dynamic tools available to the client (they must be thread-safe)
                Connection(Dispatcher& dispatcher, DynamicToolMethods methods);
                ~Connection() override;

                // juce::InterprocessConnection
                void connectionMade() override;
                void connectionLost() override;
                void messageReceived(juce::MemoryBlock const& message) override;

            private:
                Dispatcher& mDispatcher;
                DynamicToolMethods mMethods;
            };

            class Server
            : public juce::InterprocessConnectionServer
            {
            public:
                //! @brief Creates the server of the external clients
                //! @param accessor The accessor of the Neuralyzer model
                //! @param dispatcher The dispatcher of the MCP methods
                //! @param methods The methods of the dynamic tools available to the clients (they must be thread-safe)
                Server(Accessor& accessor, Dispatcher& dispatcher, DynamicToolMethods methods);
                ~Server() override;

                static bool setClaudeApplicationEnabled(bool enabled);
                static bool setCopilotApplicationEnabled(bool enabled);
                static bool setLlamaCppEnabled(bool enabled);
                static juce::File getLlamaCppConfigFile();

            protected:
                // juce::InterprocessConnectionServer
                juce::InterprocessConnection* createConnectionObject() override;

            private:
                void updateListeningState();
                static juce::String getPartielsExecutablePath();
                static bool updateClientConfigFile(juce::File const& file, std::string const& section, nlohmann::json const& entry);

                Accessor& mAccessor;
                Dispatcher& mDispatcher;
                DynamicToolMethods mMethods;
                Accessor::Listener mListener{typeid(*this).name()};
                std::vector<std::unique_ptr<Connection>> mConnections;
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
