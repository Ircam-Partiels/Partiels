#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Mcp
    {
        static auto constexpr jsonrpc = "2.0";
        nlohmann::json createError(char const* what);

        class ExecutionError
        : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        class Dispatcher
        {
        public:
            Dispatcher() = default;
            ~Dispatcher() = default;

            nlohmann::json handle(nlohmann::json const& params);
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
            Server(Dispatcher& dispatcher);
            ~Server() override;

        protected:
            // juce::InterprocessConnectionServer
            juce::InterprocessConnection* createConnectionObject() override;

        private:
            Dispatcher& mDispatcher;
            std::unique_ptr<juce::InterprocessConnection> mConnections;
        };

        namespace Host
        {
            bool run(int port);
        }
    } // namespace Mcp
} // namespace Application

ANALYSE_FILE_END
