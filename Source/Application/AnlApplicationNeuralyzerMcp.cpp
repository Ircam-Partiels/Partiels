#include "AnlApplicationNeuralyzerMcp.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Track/AnlTrackTools.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"
#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

nlohmann::json Application::Neuralyzer::Mcp::createError(std::string const& what)
{
    nlohmann::json response;
    response["jsonrpc"] = jsonrpc;
    auto& error = response["error"];
    error["code"] = -32602;
    error["message"] = what;
    return response;
}

namespace Application::Neuralyzer::Mcp
{
    static nlohmann::json callTools(nlohmann::json const& request)
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP tools/call");
        if(!request.contains("params") || !request.at("params").is_object())
        {
            return createError("The 'params' field is required and must be an object.");
        }
        auto const methodParams = request.at("params");
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", methodParams.dump());
        if(!methodParams.contains("name") || !methodParams.at("name").is_string())
        {
            return createError("The 'name' field is required and must be a string.");
        }
        auto const toolName = methodParams.at("name").get<std::string>();
        nlohmann::json response;
        response["isError"] = false;
        response["content"] = nlohmann::json::array();

        // Vamp Plugins Section
        if(toolName == "get_installed_vamp_plugins_list")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(arguments.contains("sampleRate") && !arguments.at("sampleRate").is_number())
            {
                return createError("The 'sampleRate' argument must be a number.");
            }
            auto const sampleRate = arguments.value("sampleRate", 48000.0);
            auto& scanner = Instance::get().getPluginListScanner();
            auto const [pluginMap, errors] = scanner.getPlugins(sampleRate);
            nlohmann::json plugins;
            to_json(plugins, pluginMap);
            for(auto& plugin : plugins)
            {
                // Remove large data from the response
                plugin.back().erase("defaultState");
                plugin.back().erase("parameters");
                plugin.back().erase("output");
                plugin.back().erase("extraOutputs");
                plugin.back().erase("input");
                plugin.back().erase("programs");
            }
            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["plugins"] = plugins;
            nlohmann::json errorArray = nlohmann::json::array();
            for(auto const& err : errors)
            {
                errorArray.push_back(err.toStdString());
            }
            if(!errorArray.empty())
            {
                payload["errors"] = errorArray;
                response["isError"] = true;
            }
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }

        // Document Getter Section
        if(toolName == "get_document_state")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(arguments.contains("deep") && !arguments.at("deep").is_boolean())
            {
                return createError("The 'deep' argument must be a boolean.");
            }
            auto const deep = arguments.value("deep", false);
            nlohmann::json document;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            auto const& documentLayout = documentAcsr.getAttr<Document::AttrType::layout>();
            for(auto const& groupIdentifier : documentLayout)
            {
                nlohmann::json group;
                auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupIdentifier);
                group["identifier"] = groupIdentifier;
                group["name"] = groupAcsr.getAttr<Group::AttrType::name>().toStdString();
                auto const& groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                nlohmann::json tracks = nlohmann::json::array();
                for(auto const& trackIdentifier : groupLayout)
                {
                    nlohmann::json track;
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
                    track["identifier"] = trackIdentifier;
                    track["name"] = trackAcsr.getAttr<Track::AttrType::name>().toStdString();
                    if(deep)
                    {
                        to_json(track["description"], trackAcsr.getAttr<Track::AttrType::description>());
                        to_json(track["parameters"], trackAcsr.getAttr<Track::AttrType::state>().parameters);
                        if(Track::Tools::supportsInputTrack(trackAcsr))
                        {
                            track["input"] = trackAcsr.getAttr<Track::AttrType::input>().toStdString();
                        }
                        auto const frameType = Track::Tools::getFrameType(trackAcsr);
                        if(frameType.has_value())
                        {
                            switch(frameType.value())
                            {
                                case Track::FrameType::label:
                                    track["type"] = "label";
                                    break;
                                case Track::FrameType::value:
                                    track["type"] = "value";
                                    break;
                                case Track::FrameType::vector:
                                    track["type"] = "vector";
                                    break;
                            }
                        }
                    }
                    tracks.push_back(std::move(track));
                }
                group["tracks"] = tracks;
                document["groups"].push_back(group);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = document.dump();
            response["content"].push_back(content);
            return response;
        }

        // Group Getter Section
        if(toolName == "get_group_names")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json names;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                {
                    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                    names[identifier] = groupAcsr.getAttr<Group::AttrType::name>();
                }
                else
                {
                    response["isError"] = true;
                    names[identifier] = juce::String("The group \"GROUPID\" doesn't exist.").replace("GROUPID", identifier);
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = names.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_group_layouts")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json layouts;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                {
                    auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                    layouts[identifier] = groupAcsr.getAttr<Group::AttrType::layout>();
                }
                else
                {
                    response["isError"] = true;
                    layouts[identifier] = juce::String("The group \"GROUPID\" doesn't exist.").replace("GROUPID", identifier);
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = layouts.dump();
            response["content"].push_back(content);
            return response;
        }

        // Group Setter Section
        if(toolName == "set_group_names")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("groups") || !arguments.at("groups").is_array())
            {
                return createError("The 'groups' argument is required and must be an array of objects.");
            }
            auto const& groups = arguments.at("groups");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            for(auto const& groupJson : groups)
            {
                if(!groupJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!groupJson.contains("identifier") || !groupJson.at("identifier").is_string())
                {
                    return createError("The 'identifier' field is required and must be a string.");
                }
                if(!groupJson.contains("name") || !groupJson.at("name").is_string())
                {
                    return createError("The 'name' field is required and must be a string.");
                }
                auto const identifier = juce::String(groupJson.at("identifier").get<std::string>());
                auto const name = juce::String(groupJson.at("name").get<std::string>());
                if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                {
                    auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                    groupAcsr.setAttr<Group::AttrType::name>(name, NotificationType::asynchronous);
                    results.add(juce::String("The group \"GROUPID\" has been renamed \"NAME\".").replace("GROUPID", identifier).replace("NAME", name));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The group \"GROUPID\" doesn't exist.").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Change group name (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_group_layouts")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("groups") || !arguments.at("groups").is_array())
            {
                return createError("The 'groups' argument is required and must be an array of objects.");
            }
            auto const& groups = arguments.at("groups");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            for(auto const& groupJson : groups)
            {
                if(!groupJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!groupJson.contains("identifier") || !groupJson.at("identifier").is_string())
                {
                    return createError("The 'identifier' field is required and must be a string.");
                }
                if(!groupJson.contains("layout") || !groupJson.at("layout").is_array())
                {
                    return createError("The 'layout' field is required and must be an array of strings.");
                }
                auto const identifier = juce::String(groupJson.at("identifier").get<std::string>());
                auto const layoutJson = groupJson.at("layout");
                if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                {
                    std::vector<juce::String> layout;
                    for(auto const& trackIdJson : layoutJson)
                    {
                        if(!trackIdJson.is_string())
                        {
                            return createError("The 'layout' field is required and must be an array of strings.");
                        }
                        layout.push_back(trackIdJson.get<juce::String>());
                    }
                    auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                    groupAcsr.setAttr<Group::AttrType::layout>(layout, NotificationType::asynchronous);
                    results.add(juce::String("The group \"GROUPID\" layout has been modified.").replace("GROUPID", identifier));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The group \"GROUPID\" doesn't exist.").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Change group layout (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
            response["content"].push_back(content);
            return response;
        }

        // Track Getter Section
        if(toolName == "get_track_types")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json types;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    auto const frameType = Track::Tools::getFrameType(trackAcsr);
                    if(frameType.has_value())
                    {
                        switch(frameType.value())
                        {
                            case Track::FrameType::label:
                                types[identifier] = "label";
                                break;
                            case Track::FrameType::value:
                                types[identifier] = "value";
                                break;
                            case Track::FrameType::vector:
                                types[identifier] = "vector";
                                break;
                        }
                    }
                    else
                    {
                        types[identifier] = "unknown";
                    }
                }
                else
                {
                    response["isError"] = true;
                    types[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = types.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_names")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json names;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    names[identifier] = trackAcsr.getAttr<Track::AttrType::name>();
                }
                else
                {
                    response["isError"] = true;
                    names[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = names.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_descriptions")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json descriptions;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    nlohmann::json description;
                    to_json(description, trackAcsr.getAttr<Track::AttrType::description>());
                    descriptions[identifier] = description;
                }
                else
                {
                    response["isError"] = true;
                    descriptions[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                    ;
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = descriptions.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_parameters")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json parameters;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    nlohmann::json params;
                    to_json(params, trackAcsr.getAttr<Track::AttrType::state>().parameters);
                    parameters[identifier] = params;
                }
                else
                {
                    response["isError"] = true;
                    parameters[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                    ;
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = parameters.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_inputs")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            nlohmann::json inputTracks;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    if(Track::Tools::supportsInputTrack(trackAcsr))
                    {
                        auto const inputTrack = trackAcsr.getAttr<Track::AttrType::input>();
                        inputTracks[identifier] = inputTrack;
                    }
                    else
                    {
                        inputTracks[identifier] = juce::String("The track \"TRACKID\" doesn't support input track.").replace("TRACKID", identifier);
                        ;
                    }
                }
                else
                {
                    response["isError"] = true;
                    inputTracks[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                    ;
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = inputTracks.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_compatible_inputs")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const& hierarchyManager = documentDir.getHierarchyManager();
            nlohmann::json tracks;
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                auto const compatibleTracks = hierarchyManager.getAvailableTracksFor(identifier);
                nlohmann::json trackArray = nlohmann::json::array();
                for(auto const& trackInfo : compatibleTracks)
                {
                    trackArray.push_back(trackInfo.identifier);
                }
                tracks[identifier] = trackArray;
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = tracks.dump();
            response["content"].push_back(content);
            return response;
        }

        // Track Setter Section
        if(toolName == "set_track_names")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("tracks") || !arguments.at("tracks").is_array())
            {
                return createError("The 'tracks' argument is required and must be an array of objects.");
            }
            auto const& tracks = arguments.at("tracks");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            for(auto const& trackJson : tracks)
            {
                if(!trackJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!trackJson.contains("identifier") || !trackJson.at("identifier").is_string())
                {
                    return createError("The 'identifier' field is required and must be a string.");
                }
                if(!trackJson.contains("name") || !trackJson.at("name").is_string())
                {
                    return createError("The 'name' field is required and must be a string.");
                }
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const name = juce::String(trackJson.at("name").get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::asynchronous);
                    results.add(juce::String("The track \"TRACKID\" has been renamed \"NAME\".").replace("TRACKID", identifier).replace("NAME", name));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Change track name (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_track_parameters")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("tracks") || !arguments.at("tracks").is_array())
            {
                return createError("The 'tracks' argument is required and must be an array of objects.");
            }
            auto const& tracks = arguments.at("tracks");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            for(auto const& trackJson : tracks)
            {
                if(!trackJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!trackJson.contains("identifier") || !trackJson.at("identifier").is_string())
                {
                    return createError("The 'identifier' field is required and must be a string.");
                }
                if(!trackJson.contains("parameter") || !trackJson.at("parameter").is_string())
                {
                    return createError("The 'parameter' field is required and must be a string.");
                }
                if(!trackJson.contains("value") || !trackJson.at("value").is_number())
                {
                    return createError("The 'value' field is required and must be a number.");
                }
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const parameter = trackJson.at("parameter").get<std::string>();
                auto const value = trackJson.at("value").get<float>();
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    auto const& description = trackAcsr.getAttr<Track::AttrType::description>();
                    auto const paramIt = std::find_if(description.parameters.cbegin(), description.parameters.cend(),
                                                      [&parameter](auto const& param)
                                                      {
                                                          return param.identifier == parameter;
                                                      });
                    if(paramIt != description.parameters.cend())
                    {
                        auto const clampedValue = std::clamp(value, paramIt->minValue, paramIt->maxValue);
                        auto state = trackAcsr.getAttr<Track::AttrType::state>();
                        state.parameters[parameter] = clampedValue;
                        trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::asynchronous);
                        results.add(juce::String("The parameter \"PARAM\" of the track \"TRACKID\" has been set to \"VAL\".").replace("TRACKID", identifier).replace("PARAM", parameter).replace("VAL", juce::String(clampedValue)));
                    }
                    else
                    {
                        response["isError"] = true;
                        results.add(juce::String("The parameter \"PARAM\" of the track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier).replace("PARAM", parameter));
                        break;
                    }
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Change track parameters (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_track_inputs")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("tracks") || !arguments.at("tracks").is_array())
            {
                return createError("The 'tracks' argument is required and must be an array of objects.");
            }
            auto const& tracks = arguments.at("tracks");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const& hierarchyManager = documentDir.getHierarchyManager();
            documentDir.startAction();
            juce::StringArray results;
            for(auto const& trackJson : tracks)
            {
                if(!trackJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!trackJson.contains("identifier") || !trackJson.at("identifier").is_string())
                {
                    return createError("The 'identifier' field is required and must be a string.");
                }
                if(!trackJson.contains("input") || !trackJson.at("input").is_string())
                {
                    return createError("The 'input' field is required and must be a string.");
                }
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const input = juce::String(trackJson.at("input").get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    if(!input.isEmpty() && !Document::Tools::hasTrackAcsr(documentAcsr, input))
                    {
                        response["isError"] = true;
                        results.add(juce::String("The input track \"TRACKID\" doesn't exist.").replace("TRACKID", input));
                        break;
                    }
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    if(!Track::Tools::supportsInputTrack(trackAcsr))
                    {
                        response["isError"] = true;
                        results.add(juce::String("The track \"TRACKID\" doesn't support input.").replace("TRACKID", identifier));
                        break;
                    }
                    if(!hierarchyManager.isTrackValidFor(identifier, input))
                    {
                        response["isError"] = true;
                        results.add(juce::String("The track \"TRACKID\" doesn't support input \"INPUTID\" (unsupported format or circular dependency).").replace("TRACKID", identifier).replace("INPUTID", input));
                        break;
                    }
                    trackAcsr.setAttr<Track::AttrType::input>(input, NotificationType::asynchronous);
                    if(input.isEmpty())
                    {
                        results.add(juce::String("The track \"TRACKID\" input has been disconnected.").replace("TRACKID", identifier));
                    }
                    else
                    {
                        results.add(juce::String("The track \"TRACKID\" now uses \"INPUT\" as input.").replace("TRACKID", identifier).replace("INPUT", input));
                    }
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Set track input (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
            response["content"].push_back(content);
            return response;
        }
        // Track Creation/Removal Section
        if(toolName == "create_tracks_with_plugins")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("tracks") || !arguments.at("tracks").is_array())
            {
                return createError("The 'tracks' argument is required and must be an array of objects.");
            }
            auto const& tracks = arguments.at("tracks");
            juce::StringArray results;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const references = documentDir.sanitize(NotificationType::synchronous);
            // Default group fallbacks
            auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
            if(!layout.empty())
            {
                documentDir.startAction();
                std::set<juce::String> trackIdentifiers;
                nlohmann::json created = nlohmann::json::array();
                for(auto const& trackJson : tracks)
                {
                    if(!trackJson.is_object())
                    {
                        return createError("The 'tracks' argument is required and must be an array of objects.");
                    }
                    if(!trackJson.contains("plugin") || !trackJson.at("plugin").is_object())
                    {
                        return createError("The 'plugin' field is required and must be an object.");
                    }
                    auto const& plugin = trackJson.at("plugin");
                    if(!plugin.contains("identifier") || !plugin.at("identifier").is_string() || !plugin.contains("feature") || !plugin.at("feature").is_string())
                    {
                        return createError("The 'plugin' field must contain the 'identifier' and 'feature' string fields.");
                    }
                    auto const pluginIdentifier = plugin.at("identifier").get<std::string>();
                    auto const pluginFeature = plugin.at("feature").get<std::string>();
                    if(pluginIdentifier.find(":") == std::string::npos)
                    {
                        return createError("The 'identifier' field of the 'plugin' object must be in the form 'maker:plugin' as defined by the tool 'get_installed_vamp_plugins_list'. Consider calling the tool 'get_installed_vamp_plugins_list' first to get the correct values.");
                    }
                    Plugin::Key pluginKey;
                    pluginKey.identifier = pluginIdentifier;
                    pluginKey.feature = pluginFeature;
                    auto const group = trackJson.value("group", layout.front());
                    auto const groupId = references.count(group) > 0_z ? references.at(group) : group;
                    auto const position = trackJson.value("position", 0_z);
                    auto const [result, identifier] = Tools::addPluginTrack(groupId, position, pluginKey);
                    if(result.failed())
                    {
                        response["isError"] = true;
                        results.add(juce::String("Failed to create track in group \"ID\" with plugin \"PK\": REASON Ensure the identifier and feature value correspond to the values returned by the tool 'get_installed_vamp_plugins_list' and that the group exist. Consider calling the tool 'get_installed_vamp_plugins_list' to get the correct plugin identifier and feature, and calling the tool 'get_document_state' to get the correct group identifiers.").replace("ID", groupId).replace("PK", pluginKey.identifier + ":" + pluginKey.feature).replace("REASON", result.getErrorMessage()));
                        break;
                    }
                    trackIdentifiers.insert(identifier);
                    created.push_back(identifier);
                    results.add(juce::String("Created track \"TRACKID\" with plugin \"PK\".").replace("TRACKID", identifier).replace("PK", pluginKey.identifier + ":" + pluginKey.feature));
                }
                if(!response.at("isError").get<bool>())
                {
                    Tools::revealTracks(trackIdentifiers);
                    documentDir.endAction(ActionState::newTransaction, juce::translate("Create tracks (Neuralyzer)"));
                }
                else
                {
                    documentDir.endAction(ActionState::abort);
                }
                nlohmann::json content;
                content["type"] = "text";
                nlohmann::json payload;
                payload["created"] = created;
                payload["messages"] = juce::String(results.joinIntoString("\n"));
                content["text"] = payload.dump();
                response["content"].push_back(content);
            }
            else
            {
                response["isError"] = true;
                nlohmann::json content;
                content["type"] = "text";
                content["text"] = "No groups available to create tracks";
                response["content"].push_back(content);
            }
            return response;
        }
        if(toolName == "remove_tracks")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifiers") || !arguments.at("identifiers").is_array())
            {
                return createError("The 'identifiers' argument is required and must be an array of strings.");
            }
            auto const& identifiers = arguments.at("identifiers");
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            nlohmann::json removed = nlohmann::json::array();
            for(auto const& idJson : identifiers)
            {
                if(!idJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = juce::String(idJson.get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto const result = documentDir.removeTrack(identifier, NotificationType::synchronous);
                    if(result.failed())
                    {
                        response["isError"] = true;
                        results.add(juce::String("The track \"TRACKID\" could not be removed: REASON.").replace("TRACKID", identifier).replace("REASON", result.getErrorMessage()));
                        break;
                    }
                    else
                    {
                        removed.push_back(identifier);
                    }
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Remove tracks (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["removed"] = removed;
            payload["messages"] = juce::String(results.joinIntoString("\n"));
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }
        return createError("Unknown tool " + toolName);
    }
} // namespace Application::Neuralyzer::Mcp

nlohmann::json Application::Neuralyzer::Mcp::Dispatcher::handle(nlohmann::json const& request)
{
    if(!request.contains("method") || !request.at("method").is_string())
    {
        return createError("The 'method' field is required and must be an object.");
    }
    auto const method = request.at("method").get<std::string>();
    if(method == "notifications/initialized")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP initialized notification");
        return {};
    }
    if(method == "tools/list")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP tools/list");
        return nlohmann::json::parse(AnlNeuralyzerData::tools_list_json);
    }
    if(method == "tools/call")
    {
        nlohmann::json response;
        juce::MessageManager::callSync([&]()
                                       {
                                           try
                                           {
                                               response = callTools(request);
                                           }
                                           catch(std::exception const& e)
                                           {
                                               response = createError(e.what());
                                           }
                                       });
        return response;
    }
    return createError("Unknown method: " + method);
}

Application::Neuralyzer::Mcp::Connection::Connection(Dispatcher& dispatcher)
: mDispatcher(dispatcher)
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
    auto const send = [this](nlohmann::json const& msg)
    {
        auto const response = msg.dump();
        MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client answers message: " + response);
        sendMessage(juce::MemoryBlock(response.c_str(), static_cast<size_t>(response.size())));
    };

    try
    {
        auto const text = message.toString().toStdString();
        auto const request = nlohmann::json::parse(text);
        MiscDebug("Application::Neuralyzer::Mcp::Connection", "MCP client received message: " + text);

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

Application::Neuralyzer::Mcp::Server::Server(Dispatcher& dispatcher)
: mDispatcher(dispatcher)
{
    setClaudeApplicationEnabled(true);
}

Application::Neuralyzer::Mcp::Server::~Server()
{
    setClaudeApplicationEnabled(false);
    stop();
}

juce::InterprocessConnection* Application::Neuralyzer::Mcp::Server::createConnectionObject()
{
    mConnections = std::make_unique<Connection>(mDispatcher);
    return mConnections.get();
}

bool Application::Neuralyzer::Mcp::Server::setClaudeApplicationEnabled([[maybe_unused]] bool enabled)
{
#ifdef JUCE_MAC
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile("Claude");
#else
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Claude");
#endif
    auto const file = directory.getChildFile("claude_desktop_config.json");
    nlohmann::json json;
    try
    {
        auto const content = file.loadFileAsString().toStdString();
        if(!content.empty())
        {
            json = nlohmann::json::parse(content);
        }
    }
    catch(...)
    {
    }
    if(enabled)
    {
        auto const executable = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getFullPathName();
        auto const config = juce::String(AnlNeuralyzerData::claude_desktop_config_json).replace("PARTIELS_PATH", executable);
        json["mcpServers"]["Partiels"] = nlohmann::json::parse(config.toStdString());
    }
    else if(json.contains("mcpServers"))
    {
        json["mcpServers"].erase("Partiels");
    }
    juce::String const text = json.dump();
    auto const result = file.replaceWithText(text);
    MiscWeakAssert(result);
    return result;
}

bool Application::Neuralyzer::Mcp::Host::run(int port)
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
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host connected");
        }

        void connectionLost() override
        {
            MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host disconnected");
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
                MiscDebug("Application::Neuralyzer::Mcp::Host", "MCP host received message: " + text);
            }
            catch(std::exception const& e)
            {
                auto const error = createError(e.what()).dump();
                std::cout << error << std::endl;
                MiscDebug("Application::Neuralyzer::Mcp::Host", "error: " + error);
            }
            catch(...)
            {
                auto const error = createError("Unknown").dump();
                std::cout << error << std::endl;
                MiscDebug("Application::Neuralyzer::Mcp::Host", "error: " + error);
            }
        }
    };

    MiscDebug("Application::Neuralyzer::Mcp::Host", "start");
    auto clientConnection = std::make_unique<ClientConnection>();
    if(!clientConnection->connectToSocket("127.0.0.1", port, 2000))
    {
        auto const error = createError("Failed to connect host to server").dump();
        std::cout << error << std::endl;
        MiscDebug("Application::Neuralyzer::Mcp::Host", "error: " + error);
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
                result["instructions"] = "You are an AI assistant named Neuralyzer for the **Partiels audio analysis application** ([https://github.com/Ircam-Partiels/Partiels](https://github.com/Ircam-Partiels/Partiels)).\nYou answer user questions and modify the documents representing a Partiels session. Always retrieve the current selected tracks before answering the user.";
                std::cout << response.dump() << std::endl;
                MiscDebug("Application::Neuralyzer::Mcp::Host", "initialize: " + response.dump());
            }
            else if(method == "shutdown")
            {
                MiscDebug("Application::Neuralyzer::Mcp::Host", "shutdown");
                return true;
            }
            else if(id >= 0)
            {
                MiscDebug("Application::Neuralyzer::Mcp::Host", method);
                clientConnection->sendMessage(msg);
            }
        }
        catch(std::exception const& e)
        {
            auto const error = createError(e.what()).dump();
            std::cout << error << std::endl;
            MiscDebug("Application::Neuralyzer::Mcp::Host", "error: " + error);
        }
    }
    MiscDebug("Application::Neuralyzer::Mcp::Host", "quit");
    return true;
}

ANALYSE_FILE_END
