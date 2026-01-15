#include "AnlApplicationNeuralyzerMcpDispatcher.h"
#include "AnlApplicationInstance.h"
#include <AnlNeuralyzerData.h>
#include <AnlNeuralyzerTutorialData.h>

ANALYSE_FILE_BEGIN

Application::Neuralyzer::Mcp::ProtocolError::ProtocolError(int code, std::string const& message)
: std::runtime_error(message)
, mCode(code)
{
}

int Application::Neuralyzer::Mcp::ProtocolError::getCode() const noexcept
{
    return mCode;
}

nlohmann::json Application::Neuralyzer::Mcp::createResultResponse(nlohmann::json const& id, nlohmann::json const& result)
{
    nlohmann::json response;
    response["jsonrpc"] = jsonrpc;
    response["id"] = id;
    response["result"] = result;
    return response;
}

nlohmann::json Application::Neuralyzer::Mcp::createErrorResponse(nlohmann::json const& id, int code, std::string const& message)
{
    nlohmann::json response;
    response["jsonrpc"] = jsonrpc;
    response["id"] = id;
    response["error"]["code"] = code;
    response["error"]["message"] = message;
    return response;
}

namespace Application::Neuralyzer::Mcp
{
    static nlohmann::json listResources()
    {
        nlohmann::json result;
        result["resources"] = nlohmann::json::array();
        {
            nlohmann::json resource;
            resource["uri"] = std::string("partiels://manual");
            resource["name"] = "Partiels User Manual";
            resource["title"] = "Partiels User Manual";
            resource["description"] = "The user manual for Partiels contains detailed information about how to use the application, including its features, interface, and troubleshooting tips. It serves as a comprehensive guide for users to understand and navigate the various functionalities of Partiels effectively.";
            resource["mimeType"] = "text/markdown";
            result["resources"].push_back(std::move(resource));
        }
        {
            nlohmann::json resource;
            resource["uri"] = std::string("partiels://available_plugins_list");
            resource["name"] = "Installed Analysis Vamp Plugins List";
            resource["title"] = "Installed Analysis Vamp Plugins List";
            resource["description"] = "The list of analysis Vamp plugins (keys and brief descriptions) available on the system";
            resource["mimeType"] = "text/markdown";
            result["resources"].push_back(std::move(resource));
        }
        for(auto i = 0; i < AnlNeuralyzerTutorialData::namedResourceListSize; ++i)
        {
            nlohmann::json resource;
            resource["uri"] = std::string("partiels://") + AnlNeuralyzerTutorialData::namedResourceList[i];
            resource["name"] = "Tutorial " + std::to_string(i + 1);
            resource["title"] = juce::File::createFileWithoutCheckingPath(AnlNeuralyzerTutorialData::originalFilenames[i]).getFileNameWithoutExtension();
            resource["description"] = AnlNeuralyzerTutorialData::originalFilenames[i];
            resource["mimeType"] = "text/markdown";
            result["resources"].push_back(std::move(resource));
        }
        return result;
    }

    static nlohmann::json readResource(Handlers& handlers, nlohmann::json const& request)
    {
        if(!request.contains("params") || !request.at("params").is_object())
        {
            throw ProtocolError(ErrorCode::invalidParams, "The 'params' field is required and must be an object.");
        }

        auto const& params = request.at("params");
        if(!params.contains("uri") || !params.at("uri").is_string())
        {
            throw ProtocolError(ErrorCode::invalidParams, "The 'uri' field is required and must be a string.");
        }

        auto const uri = params.at("uri").get<std::string>();
        if(uri == std::string("partiels://manual"))
        {
            nlohmann::json result;
            result["contents"] = nlohmann::json::array();
            nlohmann::json content;
            content["uri"] = uri;
            content["mimeType"] = "text/markdown";
            content["text"] = juce::String("# Partiels User Manual\nThe user manual for Partiels contains detailed information about how to use the application, including its features, interface, and troubleshooting tips. It serves as a comprehensive guide for users to understand and navigate the various functionalities of Partiels effectively\n\n") + juce::String::fromUTF8(AnlNeuralyzerData::PartielsManual_md, AnlNeuralyzerData::PartielsManual_mdSize);
            result["contents"].push_back(std::move(content));
            return result;
        }
        if(uri == std::string("partiels://available_plugins_list"))
        {
            nlohmann::json result;
            result["contents"] = nlohmann::json::array();
            nlohmann::json content;
            content["uri"] = uri;
            content["mimeType"] = "text/markdown";

            juce::String resource;
            resource << "# Analysis Vamp Plugins List\n";
            resource << "The list of analysis Vamp plugins (keys and brief descriptions) available on the system\n\n";
            nlohmann::json plugins;
            to_json(plugins, handlers.getInstalledPlugins());
            for(auto& plugin : plugins)
            {
                auto const& key = plugin.front();
                auto const& description = plugin.back();
                resource << "## " << description.value("name", juce::String("Unknown")) << " - " << key.value("feature", juce::String("Unknown")) << "\n";
                resource << "- Category: " << description.value("category", juce::String("Unknown")) << "\n";
                resource << "- Maker: " << description.value("maker", juce::String("Unknown")) << "\n";
                resource << "- Details: " << description.value("details", juce::String("Empty")) << "\n";
                resource << "- Key: " << key.dump() << "\n\n";
            }

            content["text"] = resource;
            result["contents"].push_back(std::move(content));
            return result;
        }

        for(auto i = 0; i < AnlNeuralyzerTutorialData::namedResourceListSize; ++i)
        {
            if(uri == std::string("partiels://") + AnlNeuralyzerTutorialData::namedResourceList[i])
            {
                nlohmann::json result;
                result["contents"] = nlohmann::json::array();
                nlohmann::json content;
                content["uri"] = uri;
                content["mimeType"] = "text/markdown";
                int size = 0;
                auto const resource = AnlNeuralyzerTutorialData::getNamedResource(AnlNeuralyzerTutorialData::namedResourceList[i], size);
                content["text"] = juce::String::fromUTF8(resource, size);
                result["contents"].push_back(std::move(content));
                return result;
            }
        }

        throw ProtocolError(ErrorCode::resourceNotFound, "Resource not found: " + uri);
    }
} // namespace Application::Neuralyzer::Mcp

Application::Neuralyzer::Mcp::Dispatcher::Dispatcher(Instance& instance)
: mHandlers(instance)
, mTools({// Document
          {"get_document_state", std::bind_front(&Handlers::getDocumentState, &mHandlers)},
          {"get_document_audio_file_layout", std::bind_front(&Handlers::getDocumentAudioFileLayout, &mHandlers)},
          {"set_document_audio_file_layout", std::bind_front(&Handlers::setDocumentAudioFileLayout, &mHandlers)},
          {"get_audio_file_info", std::bind_front(&Handlers::getAudioFileInfo, &mHandlers)},
          {"get_document_time_zoom", std::bind_front(&Handlers::getDocumentTimeZoom, &mHandlers)},
          {"set_document_time_zoom", std::bind_front(&Handlers::setDocumentTimeZoom, &mHandlers)},
          {"get_time_selection", std::bind_front(&Handlers::getTimeSelection, &mHandlers)},
          {"get_selected_items", std::bind_front(&Handlers::getSelectedItems, &mHandlers)},
          // Groups
          {"get_group_names", std::bind_front(&Handlers::getGroupNames, &mHandlers)},
          {"set_group_names", std::bind_front(&Handlers::setGroupNames, &mHandlers)},
          {"get_group_layouts", std::bind_front(&Handlers::getGroupLayouts, &mHandlers)},
          {"set_group_layouts", std::bind_front(&Handlers::setGroupLayouts, &mHandlers)},
          {"get_group_vertical_zoom", std::bind_front(&Handlers::getGroupVerticalZoom, &mHandlers)},
          {"set_group_vertical_zoom", std::bind_front(&Handlers::setGroupVerticalZoom, &mHandlers)},
          {"create_groups", std::bind_front(&Handlers::createGroups, &mHandlers)},
          {"remove_groups", std::bind_front(&Handlers::removeGroups, &mHandlers)},
          // Tracks
          {"create_tracks_with_plugins", std::bind_front(&Handlers::createTracksWithPlugins, &mHandlers)},
          {"create_tracks_with_files", std::bind_front(&Handlers::createTracksWithFiles, &mHandlers)},
          {"remove_tracks", std::bind_front(&Handlers::removeTracks, &mHandlers)},
          {"get_track_graphics", std::bind_front(&Handlers::getTrackGraphics, &mHandlers)},
          {"set_track_graphics", std::bind_front(&Handlers::setTrackGraphics, &mHandlers)},
          {"set_track_parameters", std::bind_front(&Handlers::setTrackParameters, &mHandlers)},
          {"set_track_inputs", std::bind_front(&Handlers::setTrackInputs, &mHandlers)},
          {"set_track_extra_thresholds", std::bind_front(&Handlers::setTrackExtraThresholds, &mHandlers)},
          {"get_track_types", std::bind_front(&Handlers::getTrackTypes, &mHandlers)},
          {"get_track_descriptions", std::bind_front(&Handlers::getTrackDescriptions, &mHandlers)},
          {"get_track_parameters", std::bind_front(&Handlers::getTrackParameters, &mHandlers)},
          {"get_track_vertical_zoom", std::bind_front(&Handlers::getTrackVerticalZoom, &mHandlers)},
          {"set_track_vertical_zoom", std::bind_front(&Handlers::setTrackVerticalZoom, &mHandlers)},
          {"get_track_value_range", std::bind_front(&Handlers::getTrackValueRange, &mHandlers)},
          {"set_track_value_range", std::bind_front(&Handlers::setTrackValueRange, &mHandlers)},
          {"get_track_inputs", std::bind_front(&Handlers::getTrackInputs, &mHandlers)},
          {"get_track_compatible_inputs", std::bind_front(&Handlers::getTrackCompatibleInputs, &mHandlers)},
          {"get_track_is_visible_in_group", std::bind_front(&Handlers::getTrackIsVisibleInGroup, &mHandlers)},
          {"set_track_is_visible_in_group", std::bind_front(&Handlers::setTrackIsVisibleInGroup, &mHandlers)},
          {"get_track_extra_thresholds", std::bind_front(&Handlers::getTrackExtraThresholds, &mHandlers)},
          {"get_track_names", std::bind_front(&Handlers::getTrackNames, &mHandlers)},
          {"set_track_names", std::bind_front(&Handlers::setTrackNames, &mHandlers)},
          // Plugins
          {"get_vamp_plugin_descriptions", std::bind_front(&Handlers::getVampPluginDescriptions, &mHandlers)},
          // Results
          {"get_track_results_raw", std::bind_front(&Handlers::getTrackResultsRaw, &mHandlers)},
          {"get_track_results_summary", std::bind_front(&Handlers::getTrackResultsSummary, &mHandlers)},
          {"set_track_results_raw", std::bind_front(&Handlers::setTrackResultsRaw, &mHandlers)},
          {"save_track_results", std::bind_front(&Handlers::saveTrackResults, &mHandlers)}})
{
}

nlohmann::json Application::Neuralyzer::Mcp::Dispatcher::callMethod(nlohmann::json const& request, DynamicToolMethods const& methods)
{
    if(!request.is_object() || !request.contains("method") || !request.at("method").is_string())
    {
        throw ProtocolError(ErrorCode::invalidRequest, "The 'method' field is required and must be a string.");
    }
    auto const method = request.at("method").get<std::string>();
    if(method == "initialize")
    {
        // The version requested by the client is used if it is supported, otherwise the latest version is used
        static std::array<std::string, 4_z> const supportedVersions{"2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05"};
        auto const params = request.value("params", nlohmann::json::object());
        auto const requestedVersion = params.is_object() && params.contains("protocolVersion") && params.at("protocolVersion").is_string() ? params.at("protocolVersion").get<std::string>() : std::string{};
        auto const isSupported = std::find(supportedVersions.cbegin(), supportedVersions.cend(), requestedVersion) != supportedVersions.cend();
        auto result = nlohmann::json::object();
        result["protocolVersion"] = isSupported ? requestedVersion : supportedVersions.front();
        result["capabilities"]["tools"] = nlohmann::json::object();
        result["capabilities"]["resources"] = nlohmann::json::object();
        result["serverInfo"]["name"] = "Partiels MCP Host";
        result["serverInfo"]["version"] = "0.1.0";
        result["instructions"] = juce::String::fromUTF8(AnlNeuralyzerData::Instructions_md, AnlNeuralyzerData::Instructions_mdSize);
        return result;
    }
    if(method == "ping")
    {
        return nlohmann::json::object();
    }
    if(method.rfind("notifications/", 0_z) == 0_z)
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP notification " + method);
        return {};
    }
    if(method == "tools/list")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP tools/list");
        auto list = nlohmann::json::parse(AnlNeuralyzerData::ToolsList_json);
        auto& tools = list["tools"];
        // Remove the tools that are not available
        auto const names = getToolNames(methods);
        auto const it = std::remove_if(tools.begin(), tools.end(), [&](auto const& tool)
                                       {
                                           return !tool.contains("name") || !tool.at("name").is_string() || names.count(tool.at("name").template get<std::string>()) == 0_z;
                                       });
        tools.erase(it, tools.end());
        // The output schemas are kept for the external clients (the successful responses of these tools
        // contain a structured content that conforms to the schemas), the agents only use the input schemas
        return list;
    }
    if(method == "resources/list")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP resources/list");
        return listResources();
    }
    if(method == "resources/read")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP resources/read");
        return readResource(mHandlers, request);
    }
    if(method == "resources/templates/list")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP resources/templates/list");
        nlohmann::json result;
        result["resourceTemplates"] = nlohmann::json::array();
        return result;
    }
    if(method == "tools/call")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP tools/call");
        if(!request.contains("params") || !request.at("params").is_object())
        {
            throw ProtocolError(ErrorCode::invalidParams, "The 'params' field is required and must be an object.");
        }
        auto const methodParams = request.at("params");
        if(!methodParams.contains("name") || !methodParams.at("name").is_string())
        {
            throw ProtocolError(ErrorCode::invalidParams, "The 'name' field is required and must be a string.");
        }

        auto const toolName = methodParams.at("name").get<std::string>();
        auto const dynamicTools = getDynamicTools(methods);
        auto const dynamicToolIt = dynamicTools.find(toolName);
        auto const toolIt = mTools.find(toolName);
        if(dynamicToolIt != dynamicTools.cend() || toolIt != mTools.cend())
        {
            // The invalid arguments and the failures are tool execution errors so the model can correct its request
            auto const arguments = methodParams.value("arguments", nlohmann::json::object());
            if(!arguments.is_object())
            {
                return createToolErrorResponse("The 'arguments' field must be an object.");
            }
            try
            {
                return dynamicToolIt != dynamicTools.cend() ? dynamicToolIt->second(arguments) : toolIt->second(arguments);
            }
            catch(std::exception const& e)
            {
                return createToolErrorResponse(e.what());
            }
        }
        throw ProtocolError(ErrorCode::invalidParams, "Unknown tool: " + toolName);
    }
    throw ProtocolError(ErrorCode::methodNotFound, "Unknown method: " + method);
}

void Application::Neuralyzer::Mcp::Dispatcher::abort()
{
    mHandlers.abort();
}

std::map<std::string, Application::Neuralyzer::Mcp::Dispatcher::tool_fn_t> Application::Neuralyzer::Mcp::Dispatcher::getDynamicTools(DynamicToolMethods const& methods)
{
    std::map<std::string, tool_fn_t> tools;
    if(methods.searchDocsFn != nullptr)
    {
        tools["search_docs"] = [this, fn = methods.searchDocsFn](nlohmann::json const& arguments)
        {
            return mHandlers.searchDocs(arguments, fn);
        };
    }
    if(methods.loadDocsFn != nullptr)
    {
        tools["load_docs"] = [this, fn = methods.loadDocsFn](nlohmann::json const& arguments)
        {
            return mHandlers.loadDocs(arguments, fn);
        };
    }
    if(methods.readFilesFn != nullptr)
    {
        tools["read_files"] = [this, fn = methods.readFilesFn](nlohmann::json const& arguments)
        {
            return mHandlers.readFiles(arguments, fn);
        };
    }
    return tools;
}

std::set<std::string> Application::Neuralyzer::Mcp::Dispatcher::getToolNames(DynamicToolMethods const& methods)
{
    std::set<std::string> names;
    for(auto const& tool : mTools)
    {
        names.insert(tool.first);
    }
    for(auto const& tool : getDynamicTools(methods))
    {
        names.insert(tool.first);
    }
    return names;
}

std::string Application::Neuralyzer::Mcp::Dispatcher::getUuid()
{
    auto const instruction = [this]()
    {
        nlohmann::json request;
        request["method"] = "initialize";
        auto const response = callMethod(request);
        return response.value("instructions", std::string{});
    }();

    auto const tools = [this]()
    {
        nlohmann::json request;
        request["method"] = "tools/list";
        auto const response = callMethod(request);
        if(response.contains("tools"))
        {
            return response.at("tools").dump();
        }
        return std::string{};
    }();

    auto const state = juce::String::fromUTF8(instruction.c_str()) + juce::String::fromUTF8(tools.c_str());
    juce::MD5 md5(state.toUTF8());
    return md5.toHexString().toStdString();
}

std::string Application::Neuralyzer::Mcp::Dispatcher::getInstructions()
{
    nlohmann::json request;
    request["method"] = "initialize";
    auto const response = callMethod(request);
    auto instruction = response.value("instructions", juce::String{});
    juce::MessageManager::callSync([&]()
                                   {
                                       instruction += "\nOS Name: " + juce::SystemStats::getOperatingSystemName() + "\n";
                                       if(auto* mapping = juce::LocalisedStrings::getCurrentMappings())
                                       {
                                           instruction += "Language: " + mapping->getLanguageName() + "\n";
                                       }
                                   });
    return instruction.toStdString();
}

std::vector<std::pair<std::string, std::string>> Application::Neuralyzer::Mcp::Dispatcher::getResources()
{
    nlohmann::ordered_json listRequest;
    listRequest["method"] = "resources/list";
    auto const listResults = callMethod(listRequest);
    MiscWeakAssert(listResults.contains("resources") && listResults.at("resources").is_array());
    if(!listResults.contains("resources") || !listResults.at("resources").is_array())
    {
        return {};
    }
    std::vector<std::pair<std::string, std::string>> resources;
    for(auto const& resource : listResults.at("resources"))
    {
        MiscWeakAssert(resource.contains("name") && resource.at("name").is_string() && resource.contains("uri") && resource.at("uri").is_string());
        if(resource.contains("title") && resource.at("title").is_string() && resource.contains("uri") && resource.at("uri").is_string())
        {
            nlohmann::json readRequest;
            readRequest["method"] = "resources/read";
            readRequest["params"]["uri"] = resource.value("uri", std::string{});
            auto const contentResult = callMethod(readRequest);
            MiscWeakAssert(contentResult.contains("contents") && contentResult.at("contents").is_array());
            if(contentResult.contains("contents") && contentResult.at("contents").is_array())
            {
                auto const title = resource.at("title").get<std::string>();
                for(auto const& content : contentResult.at("contents"))
                {
                    MiscWeakAssert(content.contains("text") && content.at("text").is_string());
                    if(content.contains("text") && content.at("text").is_string())
                    {
                        resources.push_back(std::make_pair(title, content.at("text").get<std::string>()));
                    }
                }
            }
        }
    }
    return resources;
}

std::vector<common_chat_tool> Application::Neuralyzer::Mcp::Dispatcher::getToolList(DynamicToolMethods const& methods)
{
    nlohmann::json request;
    request["method"] = "tools/list";
    auto const response = callMethod(request, methods);
    MiscWeakAssert(response.contains("tools") && response.at("tools").is_array());
    if(!response.contains("tools") || !response.at("tools").is_array())
    {
        return {};
    }
    std::vector<common_chat_tool> ctools;
    for(auto const& tool : response.at("tools"))
    {
        MiscWeakAssert(tool.contains("name") && tool.at("name").is_string() && tool.contains("description") && tool.at("description").is_string());
        if(tool.contains("name") && tool.at("name").is_string() && tool.contains("description") && tool.at("description").is_string())
        {
            common_chat_tool ctool;
            ctool.name = tool.at("name").get<std::string>();
            ctool.description = tool.at("description").get<std::string>();
            ctool.parameters = tool.contains("inputSchema") ? tool.at("inputSchema").dump() : std::string({});
            ctools.push_back(std::move(ctool));
        }
    }
    return ctools;
}

common_chat_msg Application::Neuralyzer::Mcp::Dispatcher::callTool(DynamicToolMethods const& methods, common_chat_tool_call const& toolCall)
{
    common_chat_msg message;
    message.role = "tool";
    message.tool_name = toolCall.name;
    message.tool_call_id = toolCall.id;
    auto const createProtocolErrorContent = [](int code, std::string const& errorMessage)
    {
        return nlohmann::json{{"error", {{"code", code}, {"message", errorMessage}}}}.dump();
    };
    try
    {
        nlohmann::json request;
        request["method"] = "tools/call";
        request["params"]["name"] = toolCall.name;
        request["params"]["arguments"] = toolCall.arguments.empty() ? nlohmann::json::object() : nlohmann::json::parse(toolCall.arguments);
        auto response = callMethod(request, methods);
        // The structured content duplicates the text content
        response.erase("structuredContent");
        message.content = response.dump();
    }
    catch(nlohmann::json::parse_error const& e)
    {
        // The model can correct the arguments of the tool
        message.content = createToolErrorResponse(std::string("The arguments are not a valid JSON object: ") + e.what()).dump();
    }
    catch(ProtocolError const& e)
    {
        message.content = createProtocolErrorContent(e.getCode(), e.what());
    }
    catch(std::exception const& e)
    {
        message.content = createProtocolErrorContent(ErrorCode::internalError, std::string("Exception during execution: ") + e.what());
    }
    catch(...)
    {
        message.content = createProtocolErrorContent(ErrorCode::internalError, "Exception during execution: Unknown error");
    }
    return message;
}

namespace Application::Neuralyzer::Mcp
{
    class DispatcherUnitTest
    : public juce::UnitTest
    {
    public:
        DispatcherUnitTest()
        : juce::UnitTest("Mcp Dispatcher", "Application")
        {
        }

        ~DispatcherUnitTest() override = default;

        void runTest() override
        {
            beginTest("tools list");
            {
                auto const list = nlohmann::json::parse(AnlNeuralyzerData::ToolsList_json);
                expect(list.contains("tools") && list.at("tools").is_array());
                std::set<std::string> listedNames;
                for(auto const& tool : list.at("tools"))
                {
                    expect(tool.contains("name") && tool.at("name").is_string());
                    expect(tool.contains("description") && tool.at("description").is_string());
                    expect(tool.contains("inputSchema") && tool.at("inputSchema").is_object());
                    // The structured content of a response must be an object
                    expect(!tool.contains("outputSchema") || tool.at("outputSchema").value("type", std::string{}) == "object");
                    auto const name = tool.value("name", std::string{});
                    expect(listedNames.insert(name).second, "The tool " + name + " is listed several times");
                }

                DynamicToolMethods methods;
                methods.readFilesFn = []([[maybe_unused]] std::vector<std::string> const& filePaths)
                {
                    return nlohmann::json{};
                };
                methods.searchDocsFn = []([[maybe_unused]] std::string const& query, [[maybe_unused]] size_t maxNumResources)
                {
                    return nlohmann::json{};
                };
                methods.loadDocsFn = []([[maybe_unused]] std::vector<std::string> const& ids)
                {
                    return nlohmann::json{};
                };
                Dispatcher dispatcher(Instance::get());
                auto const registeredNames = dispatcher.getToolNames(methods);
                for(auto const& name : listedNames)
                {
                    expect(registeredNames.count(name) > 0_z, "The tool " + name + " is listed but not registered");
                }
                for(auto const& name : registeredNames)
                {
                    expect(listedNames.count(name) > 0_z, "The tool " + name + " is registered but not listed");
                }
                expect(dispatcher.getToolNames({}).count("read_files") == 0_z);
            }

            beginTest("protocol");
            {
                Dispatcher dispatcher(Instance::get());
                auto const call = [&](nlohmann::json const& request)
                {
                    return dispatcher.callMethod(request);
                };
                auto const getErrorCode = [&](nlohmann::json const& request)
                {
                    try
                    {
                        call(request);
                    }
                    catch(ProtocolError const& e)
                    {
                        return e.getCode();
                    }
                    return 0;
                };

                auto const initialize = [&](std::string const& version)
                {
                    return call({{"method", "initialize"}, {"params", {{"protocolVersion", version}}}}).value("protocolVersion", std::string{});
                };
                expectEquals(initialize("2025-06-18"), std::string("2025-06-18"));
                expectEquals(initialize("1999-01-01"), std::string("2025-11-25"));
                expect(call({{"method", "ping"}}) == nlohmann::json::object());
                expect(call({{"method", "notifications/initialized"}}).is_null());
                expect(call({{"method", "notifications/cancelled"}}).is_null());
                expectEquals(getErrorCode({{"id", 1}}), ErrorCode::invalidRequest);
                expectEquals(getErrorCode({{"method", "unknown"}}), ErrorCode::methodNotFound);
                expectEquals(getErrorCode({{"method", "tools/call"}, {"params", {{"name", "unknown"}}}}), ErrorCode::invalidParams);
                expectEquals(getErrorCode({{"method", "resources/read"}, {"params", {{"uri", "partiels://unknown"}}}}), ErrorCode::resourceNotFound);
            }

            beginTest("tool errors");
            {
                Dispatcher dispatcher(Instance::get());
                // The invalid arguments are tool execution errors
                auto const response = dispatcher.callMethod({{"method", "tools/call"}, {"params", {{"name", "get_track_names"}, {"arguments", {{"identifiers", 1}}}}}});
                expect(response.value("isError", false));
                expect(response.contains("content") && response.at("content").is_array() && !response.at("content").empty());

                common_chat_tool_call toolCall;
                toolCall.name = "get_track_names";
                toolCall.arguments = "{invalid";
                auto const message = dispatcher.callTool({}, toolCall);
                auto const content = nlohmann::json::parse(message.content);
                expect(content.value("isError", false));

                toolCall.name = "unknown";
                toolCall.arguments = "{}";
                auto const unknownContent = nlohmann::json::parse(dispatcher.callTool({}, toolCall).content);
                expect(unknownContent.contains("error") && unknownContent.at("error").value("code", 0) == ErrorCode::invalidParams);
            }
        }
    };

    static DispatcherUnitTest dispatcherUnitTest;
} // namespace Application::Neuralyzer::Mcp

ANALYSE_FILE_END
