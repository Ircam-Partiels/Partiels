#include "AnlApplicationNeuralyzerMcp.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::Mcp::Connection::Connection(Dispatcher& dispatcher)
// The messages are received on the thread of the connection so the message thread is only used to access the document
: juce::InterprocessConnection(false)
, mDispatcher(dispatcher)
{
}

Application::Neuralyzer::Mcp::Connection::~Connection()
{
    disconnect();
}

void Application::Neuralyzer::Mcp::Connection::connectionMade()
{
    MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client connected");
}

void Application::Neuralyzer::Mcp::Connection::connectionLost()
{
    MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client disconnected");
}

void Application::Neuralyzer::Mcp::Connection::messageReceived(juce::MemoryBlock const& message)
{
    auto const send = [this](nlohmann::json const& response)
    {
        auto const text = response.dump();
        MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client answers message: " + text);
        sendMessage(juce::MemoryBlock(text.c_str(), text.size()));
    };

    auto const text = message.toString().toStdString();
    MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client received message: " + text);
    nlohmann::json request;
    try
    {
        request = nlohmann::json::parse(text);
    }
    catch(std::exception const& e)
    {
        send(createErrorResponse(nullptr, ErrorCode::parseError, std::string("Parse error: ") + e.what()));
        return;
    }
    if(!request.is_object())
    {
        send(createErrorResponse(nullptr, ErrorCode::invalidRequest, "The request must be an object."));
        return;
    }

    // The notifications (without identifier) don't expect any response
    auto const hasId = request.contains("id");
    auto const id = hasId ? request.at("id") : nlohmann::json{};
    try
    {
        auto const result = mDispatcher.callMethod(request);
        if(hasId)
        {
            send(createResultResponse(id, result));
        }
    }
    catch(ProtocolError const& e)
    {
        if(hasId)
        {
            send(createErrorResponse(id, e.getCode(), e.what()));
        }
    }
    catch(std::exception const& e)
    {
        if(hasId)
        {
            send(createErrorResponse(id, ErrorCode::internalError, e.what()));
        }
    }
    catch(...)
    {
        if(hasId)
        {
            send(createErrorResponse(id, ErrorCode::internalError, "Unknown error"));
        }
    }
}

Application::Neuralyzer::Mcp::Server::Server(Accessor& accessor, Dispatcher& dispatcher)
: mAccessor(accessor)
, mDispatcher(dispatcher)
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelInfo:
            case AttrType::effectiveState:
            case AttrType::agentBackend:
            case AttrType::enableThinking:
            case AttrType::showAdvancedSettings:
                break;
            case AttrType::mcpForClaudeApp:
            {
                setClaudeApplicationEnabled(acsr.getAttr<AttrType::mcpForClaudeApp>());
                updateListeningState();
                break;
            }
            case AttrType::mcpForCopilotApp:
            {
                setCopilotApplicationEnabled(acsr.getAttr<AttrType::mcpForCopilotApp>());
                updateListeningState();
                break;
            }
            case AttrType::mcpForLlamaCpp:
            {
                setLlamaCppEnabled(acsr.getAttr<AttrType::mcpForLlamaCpp>());
                updateListeningState();
                break;
            }
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::Neuralyzer::Mcp::Server::~Server()
{
    mAccessor.removeListener(mListener);
    if(mAccessor.getAttr<AttrType::mcpForClaudeApp>())
    {
        setClaudeApplicationEnabled(false);
    }
    if(mAccessor.getAttr<AttrType::mcpForCopilotApp>())
    {
        setCopilotApplicationEnabled(false);
    }
    if(mAccessor.getAttr<AttrType::mcpForLlamaCpp>())
    {
        setLlamaCppEnabled(false);
    }
    stop();
    mConnections.clear();
}

void Application::Neuralyzer::Mcp::Server::updateListeningState()
{
    auto const shouldListen = mAccessor.getAttr<AttrType::mcpForClaudeApp>() || mAccessor.getAttr<AttrType::mcpForCopilotApp>() || mAccessor.getAttr<AttrType::mcpForLlamaCpp>();
    auto const isListening = getBoundPort() >= 0;
    if(shouldListen == isListening)
    {
        return;
    }
    if(shouldListen)
    {
        // Only accept local connections: the MCP server has no authentication
        if(!beginWaitingForSocket(Host::defaultPort, "127.0.0.1"))
        {
            MiscDebug("Application::Neuralyzer::Mcp::Server", "Failed to start MCP server on port " + juce::String(Host::defaultPort));
        }
    }
    else
    {
        stop();
        mConnections.clear();
    }
}

juce::InterprocessConnection* Application::Neuralyzer::Mcp::Server::createConnectionObject()
{
    // Several clients can be connected at the same time (Claude and Copilot for example)
    // The connections closed by their clients are removed when a new client connects
    std::erase_if(mConnections, [](auto const& connection)
                  {
                      return !connection->isConnected();
                  });
    mConnections.push_back(std::make_unique<Connection>(mDispatcher));
    return mConnections.back().get();
}

juce::String Application::Neuralyzer::Mcp::Server::getPartielsExecutablePath()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getFullPathName();
}

bool Application::Neuralyzer::Mcp::Server::updateClientConfigFile(juce::File const& file, std::string const& section, nlohmann::json const& entry)
{
    static auto constexpr serverName = "Partiels";
    try
    {
        auto json = file.existsAsFile() ? nlohmann::json::parse(file.loadFileAsString().toStdString()) : nlohmann::json::object();
        if(entry.is_null() && json.contains(section))
        {
            json[section].erase(serverName);
        }
        else if(!entry.is_null())
        {
            json[section][serverName] = entry;
        }
        return file.getParentDirectory().createDirectory().wasOk() && file.replaceWithText(juce::String(json.dump(2)), false, false, nullptr);
    }
    catch(...)
    {
        return false;
    }
}

bool Application::Neuralyzer::Mcp::Server::setClaudeApplicationEnabled(bool enabled)
{
#ifdef JUCE_MAC
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile("Claude");
#else
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Claude");
#endif
    auto const file = directory.getChildFile("claude_desktop_config.json");
    auto const entry = enabled ? nlohmann::json{{"command", getPartielsExecutablePath().toStdString()}, {"args", nlohmann::json::array({Neuralyzer::Mcp::Host::defaultArg})}, {"timeout", 300000}} : nlohmann::json{};
    return updateClientConfigFile(file, "mcpServers", entry);
}

bool Application::Neuralyzer::Mcp::Server::setCopilotApplicationEnabled(bool enabled)
{
#ifdef JUCE_MAC
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("Application Support")
                               .getChildFile("Code")
                               .getChildFile("User");
#else
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("Code")
                               .getChildFile("User");
#endif
    auto const file = directory.getChildFile("mcp.json");
    auto const entry = enabled ? nlohmann::json{{"type", "stdio"}, {"command", getPartielsExecutablePath().toStdString()}, {"args", nlohmann::json::array({Neuralyzer::Mcp::Host::defaultArg})}} : nlohmann::json{};
    return updateClientConfigFile(file, "servers", entry);
}

juce::File Application::Neuralyzer::Mcp::Server::getLlamaCppConfigFile()
{
    // llama-server has no default location for its MCP configuration: the file must be given with --mcp-servers-config
    auto const root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("llama_mcp_config.json");
}

bool Application::Neuralyzer::Mcp::Server::setLlamaCppEnabled(bool enabled)
{
    auto const entry = enabled ? nlohmann::json{{"command", getPartielsExecutablePath().toStdString()}, {"args", nlohmann::json::array({Neuralyzer::Mcp::Host::defaultArg})}, {"timeout_ms", 300000}} : nlohmann::json{};
    return updateClientConfigFile(getLlamaCppConfigFile(), "mcpServers", entry);
}

bool Application::Neuralyzer::Mcp::Host::run(int port)
{
    // Writes a message on the standard output (from the main thread or from the thread of the connection)
    static auto const write = [](std::string const& text)
    {
        static std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        std::cout << juce::String(text).trimEnd() << std::endl;
    };

    class ClientConnection
    : public juce::InterprocessConnection
    {
    public:
        ClientConnection()
        : juce::InterprocessConnection(false)
        {
        }

        ~ClientConnection() override
        {
            disconnect();
        }

        // juce::InterprocessConnection
        void connectionMade() override
        {
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host connected");
        }

        void connectionLost() override
        {
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host disconnected");
            // The pending requests will never be answered by the server
            std::set<std::string> pendingRequests;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                pendingRequests.swap(mPendingRequests);
            }
            for(auto const& id : pendingRequests)
            {
                write(createErrorResponse(nlohmann::json::parse(id), ErrorCode::internalError, "The connection to Partiels has been lost.").dump());
            }
        }

        void messageReceived(juce::MemoryBlock const& message) override
        {
            auto const text = message.toString().toStdString();
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host received message: " + text);
            try
            {
                auto const response = nlohmann::json::parse(text);
                if(response.is_object() && response.contains("id"))
                {
                    std::unique_lock<std::mutex> lock(mMutex);
                    mPendingRequests.erase(response.at("id").dump());
                }
            }
            catch(...)
            {
            }
            write(text);
        }

        bool send(nlohmann::json const& message)
        {
            auto const id = message.is_object() && message.contains("id") ? std::optional<std::string>(message.at("id").dump()) : std::nullopt;
            if(id.has_value())
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mPendingRequests.insert(id.value());
            }
            auto const text = message.dump();
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host send message: " + text);
            if(sendMessage(juce::MemoryBlock(text.c_str(), text.size())))
            {
                return true;
            }
            if(id.has_value())
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mPendingRequests.erase(id.value());
            }
            return false;
        }

    private:
        std::mutex mMutex;
        std::set<std::string> mPendingRequests;
    };

    MiscDebug("Application::Neuralyzer::Mcp::Host", "start");
    // The host (re)connects to the server when it receives a message so it works even if Partiels is launched later
    ClientConnection clientConnection;
    auto const connect = [&]()
    {
        return clientConnection.isConnected() || clientConnection.connectToSocket("127.0.0.1", port, 1000);
    };
    connect();

    std::string line;
    while(std::getline(std::cin, line))
    {
        if(juce::String(line).trim().isEmpty())
        {
            continue;
        }
        nlohmann::json message;
        try
        {
            message = nlohmann::json::parse(line);
        }
        catch(std::exception const& e)
        {
            write(createErrorResponse(nullptr, ErrorCode::parseError, std::string("Parse error: ") + e.what()).dump());
            continue;
        }
        if(message.is_object() && message.value("method", std::string{}) == "shutdown")
        {
            MiscDebug("Application::Neuralyzer::Mcp::Host", "shutdown");
            return true;
        }
        if(!connect() || !clientConnection.send(message))
        {
            if(message.is_object() && message.contains("id"))
            {
                write(createErrorResponse(message.at("id"), ErrorCode::internalError, "Partiels is not running or its MCP server is not enabled.").dump());
            }
        }
    }
    MiscDebug("Application::Neuralyzer::Mcp::Host", "quit");
    return true;
}

ANALYSE_FILE_END
