#include "AnlApplicationMcp.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

nlohmann::json Application::Mcp::createError(char const* what)
{
    nlohmann::json response;
    response["jsonrpc"] = jsonrpc;
    auto& error = response["error"];
    error["code"] = -32602;
    error["message"] = what;
    return response;
}

nlohmann::json Application::Mcp::Dispatcher::handle(nlohmann::json const& request)
{
    auto const method = request.at("method").get<std::string>();
    if(method == "notifications/initialized")
    {
        MiscDebug("Application::Mcp::Dispatcher", "Received MCP initialized notification");
        return {};
    }
    if(method == "tools/list")
    {
        MiscDebug("Application::Mcp::Dispatcher", "Received MCP tools/list");
        nlohmann::json response;
        response["tools"] = nlohmann::json::array();
        {
            nlohmann::json tool;
            tool["name"] = "get_selected_tracks";
            tool["description"] = "Query current selected tracks";
            {
                auto& schema = tool["inputSchema"];
                schema["type"] = "object";
                schema["properties"] = nlohmann::json::object();
                schema["required"] = nlohmann::json::array();
            }
            {
                auto& schema = tool["outputSchema"];
                schema["type"] = "object";
                schema["properties"] = nlohmann::json::object();
                auto& identifiers = schema["properties"]["identifiers"] = nlohmann::json::object();
                identifiers["type"] = "array";
                identifiers["description"] = "The unique identifiers of the selected tracks";
            }
            response["tools"].push_back(tool);
        }
        {
            nlohmann::json tool;
            tool["name"] = "get_track_name";
            tool["description"] = "Query the name of a track";
            {
                auto& schema = tool["inputSchema"];
                schema["type"] = "object";
                schema["properties"] = nlohmann::json::object();
                auto& identifier = tool["inputSchema"]["properties"]["identifier"];
                identifier["type"] = "string";
                identifier["description"] = "The unique identifier of the track";
                schema["required"] = nlohmann::json::array({"identifier"});
            }
            {
                auto& schema = tool["outputSchema"];
                schema["type"] = "object";
                schema["properties"] = nlohmann::json::object();
                auto& name = schema["properties"]["name"] = nlohmann::json::object();
                name["type"] = "string";
                name["description"] = "The name of the track";
            }
            response["tools"].push_back(tool);
        }
        {
            nlohmann::json tool;
            tool["name"] = "set_track_name";
            tool["description"] = "Change the name of a track";
            {
                auto& schema = tool["inputSchema"];
                schema["type"] = "object";
                schema["properties"] = nlohmann::json::object();
                auto& identifier = tool["inputSchema"]["properties"]["identifier"];
                identifier["type"] = "string";
                identifier["description"] = "The unique identifier of the track";
                auto& name = tool["inputSchema"]["properties"]["name"];
                name["type"] = "string";
                name["description"] = "The new name of the track";
                schema["required"] = nlohmann::json::array({"identifier"});
            }
            response["tools"].push_back(tool);
        }
        return response;
    }
    if(method == "tools/call")
    {
        MiscDebug("Application::Mcp::Dispatcher", "Received MCP tools/call");
        auto const methodParams = request.at("params");
        auto const methodName = methodParams.at("name").get<std::string>();
        if(methodName == "get_selected_tracks")
        {
            nlohmann::json response;
            response["isError"] = false;
            response["content"] = nlohmann::json::array();
            {
                auto const& documentAcsr = Instance::get().getDocumentAccessor();
                auto const selectedItems = Document::Selection::getItems(documentAcsr);
                nlohmann::json tracks;
                for(auto const& item : std::get<1>(selectedItems))
                {
                    tracks.push_back(item.toStdString());
                }

                nlohmann::json content;
                content["type"] = "text";
                content["text"] = tracks.dump();
                response["content"].push_back(content);
            }
            return response;
        }
        if(methodName == "get_track_name")
        {
            auto const arguments = methodParams.at("arguments");
            auto const identifier = juce::String(arguments.at("identifier").get<std::string>());
            nlohmann::json response;
            response["isError"] = false;
            response["content"] = nlohmann::json::array();
            {
                nlohmann::json content;
                content["type"] = "text";
                auto const& documentAcsr = Instance::get().getDocumentAccessor();
                if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    content["text"] = "The track \"" + identifier + "\" doesn't exist.";
                    response["isError"] = true;
                }
                else
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    content["text"] = trackAcsr.getAttr<Track::AttrType::name>();
                }
                response["content"].push_back(content);
            }
            return response;
        }
        if(methodName == "set_track_name")
        {
            auto const arguments = methodParams.at("arguments");
            auto const identifier = juce::String(arguments.at("identifier").get<std::string>());
            auto const name = juce::String(arguments.at("name").get<std::string>());
            nlohmann::json response;
            response["isError"] = false;
            response["content"] = nlohmann::json::array();
            {
                nlohmann::json content;
                content["type"] = "text";
                auto& documentAcsr = Instance::get().getDocumentAccessor();
                if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    content["text"] = "The track \"" + identifier + "\" doesn't exist.";
                    response["isError"] = true;
                }
                else
                {
                    auto& documentDir = Instance::get().getDocumentDirector();
                    documentDir.startAction();
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
                    documentDir.endAction(ActionState::newTransaction, juce::translate("Change track name via MCP"));
                    content["text"] = "The track \"" + identifier + "\" has been renamed \"" + name + "\"";
                }
                response["content"].push_back(content);
            }
            return response;
        }
        throw std::runtime_error("Unknown tool");
    }
    throw std::runtime_error("Unknown method");
}

Application::Mcp::Connection::Connection(Dispatcher& dispatcher)
: mDispatcher(dispatcher)
{
}

Application::Mcp::Connection::~Connection()
{
    disconnect();
}

void Application::Mcp::Connection::connectionMade()
{
    MiscDebug("Application::Mcp::Connection", "MCP client connected");
}

void Application::Mcp::Connection::connectionLost()
{
    MiscDebug("Application::Mcp::Connection", "MCP client disconnected");
}

void Application::Mcp::Connection::messageReceived(juce::MemoryBlock const& message)
{
    auto const send = [this](nlohmann::json const& msg)
    {
        auto const response = msg.dump();
        MiscDebug("Application::Mcp::Connection", "MCP client answers message: " + response);
        sendMessage(juce::MemoryBlock(response.c_str(), static_cast<size_t>(response.size())));
    };

    try
    {
        auto const text = message.toString().toStdString();
        auto const request = nlohmann::json::parse(text);
        MiscDebug("Application::Mcp::Connection", "MCP client received message: " + text);

        // Dispatch the request and send back the dumped JSON response
        auto const result = mDispatcher.handle(request);
        nlohmann::json response;
        response["result"] = result;
        response["id"] = request.value("id", -1);
        response["jsonrpc"] = jsonrpc;
        send(response);
    }
    catch(std::exception const& e)
    {
        send(createError(e.what()));
    }
    catch(...)
    {
        send(createError("Unknown"));
    }
}

Application::Mcp::Server::Server(Dispatcher& dispatcher)
: mDispatcher(dispatcher)
{
}

Application::Mcp::Server::~Server()
{
    stop();
}

juce::InterprocessConnection* Application::Mcp::Server::createConnectionObject()
{
    mConnections = std::make_unique<Connection>(mDispatcher);
    return mConnections.get();
}

bool Application::Mcp::Host::run(int port)
{
    class ClientConnection
    : public juce::InterprocessConnection
    {
    public:
        ClientConnection()
        : juce::InterprocessConnection(false)
        {
            disconnect();
        }

        ~ClientConnection() override = default;

        // juce::InterprocessConnection
        void connectionMade() override
        {
            MiscDebug("Application::Mcp::Host", "MCP host connected");
        }

        void connectionLost() override
        {
            MiscDebug("Application::Mcp::Host", "MCP host disconnected");
        }

        void sendMessage(nlohmann::json const& message)
        {
            auto const text = message.dump() + "\n";
            juce::InterprocessConnection::sendMessage(juce::MemoryBlock(text.c_str(), static_cast<size_t>(text.size())));
        }

        void messageReceived(juce::MemoryBlock const& message) override
        {
            try
            {
                auto const text = message.toString().toStdString();
                std::cout << text << std::endl;
                MiscDebug("Application::Mcp::Host", "MCP host received message: " + text);
            }
            catch(std::exception const& e)
            {
                auto const error = createError(e.what()).dump();
                std::cout << error << std::endl;
                MiscDebug("Application::Mcp::Host", "error: " + error);
            }
            catch(...)
            {
                auto const error = createError("Unkown").dump();
                std::cout << error << std::endl;
                MiscDebug("Application::Mcp::Host", "error: " + error);
            }
        }
    };

    MiscDebug("Application::Mcp::Host", "start");
    auto clientConnection = std::make_unique<ClientConnection>();
    if(!clientConnection->connectToSocket("127.0.0.1", port, 2000))
    {
        auto const error = createError("Failed to connect host to server").dump();
        std::cout << error << std::endl;
        MiscDebug("Application::Mcp::Host", "error: " + error);
        return false;
    }

    std::string line;
    while(std::getline(std::cin, line))
    {
        if(line.empty())
        {
            continue;
        }
        try
        {
            auto const msg = nlohmann::json::parse(line);
            auto const method = msg.value("method", "");
            auto const id = msg.value("id", -1);

            if(method == "initialize")
            {
                nlohmann::json response;
                response["jsonrpc"] = jsonrpc;
                response["id"] = id;
                auto& result = response["result"];
                result["protocolVersion"] = "2025-06-18";
                result["capabilities"]["tools"] = nlohmann::json::object();
                result["serverInfo"]["name"] = "Partiels MCP Host";
                result["serverInfo"]["version"] = "0.1.0";
                result["instructions"] = "You are an AI assistant named Neuralyzer for the **Partiels audio analysis application** ([https://github.com/Ircam-Partiels/Partiels](https://github.com/Ircam-Partiels/Partiels)).\nYou answer user questions and modify the documents representing a Partiels session.";
                std::cout << response.dump() << std::endl;
                MiscDebug("Application::Mcp::Host", "initialize: " + response.dump());
            }
            else if(method == "shutdown")
            {
                MiscDebug("Application::Mcp::Host", "shutdown");
                return 0;
            }
            else if(id >= 0)
            {
                MiscDebug("Application::Mcp::Host", method);
                clientConnection->sendMessage(msg);
            }
        }
        catch(std::exception const& e)
        {
            auto const error = createError(e.what()).dump();
            std::cout << error << std::endl;
            MiscDebug("Application::Mcp::Host", "error: " + error);
        }
    }
    MiscDebug("Application::Mcp::Host", "quit");
    return 0;
}

ANALYSE_FILE_END
