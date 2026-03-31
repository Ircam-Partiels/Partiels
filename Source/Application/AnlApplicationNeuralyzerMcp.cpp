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
    // Converts a juce::Colour to a string in the format "#RRGGBBAA"
    static std::string colourToString(juce::Colour const& colour)
    {
        auto const value = (static_cast<uint32_t>(colour.getRed()) << 24u | static_cast<uint32_t>(colour.getGreen()) << 16u | static_cast<uint32_t>(colour.getBlue()) << 8u | static_cast<uint32_t>(colour.getAlpha()));
        std::array<char, 10> buf;
        std::snprintf(buf.data(), buf.size(), "#%08x", value);
        return std::string(buf.data());
    }

    // Converts a string in the format "#RRGGBBAA" to a juce::Colour
    static std::optional<juce::Colour> stringToColour(std::string str)
    {
        str = juce::String(str).unquoted().toStdString();
        if(str[0] != '#')
        {
            return {};
        }
        auto value = std::stoul(str.substr(1), nullptr, 16);
        uint8_t r, g, b, a;
        if(str.size() == 7)
        {
            r = (value >> 16) & 0xFF;
            g = (value >> 8) & 0xFF;
            b = (value >> 0) & 0xFF;
            return juce::Colour::fromRGBA(r, g, b, 255);
        }
        else if(str.size() == 9)
        {
            r = (value >> 24) & 0xFF;
            g = (value >> 16) & 0xFF;
            b = (value >> 8) & 0xFF;
            a = (value >> 0) & 0xFF;
            return juce::Colour::fromRGBA(r, g, b, a);
        }
        return {};
    }

    // Removes large data from plugin description to keep the response lightweight
    static void sanitizePluginDescription(nlohmann::json& description)
    {
        // Remove binNames from output and extraOutputs
        if(description.contains("output") && description.at("output").is_object())
        {
            description["output"].erase("binNames");
        }
    }

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
        if(toolName == "get_vamp_plugin_descriptions")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("keys") || !arguments.at("keys").is_array())
            {
                return createError("The 'keys' argument is required and must be an array.");
            }
            if(arguments.contains("sampleRate") && !arguments.at("sampleRate").is_number())
            {
                return createError("The 'sampleRate' argument must be a number.");
            }
            auto const sampleRate = arguments.value("sampleRate", 48000.0);
            auto& scanner = Instance::get().getPluginListScanner();
            auto const [pluginMap, scanErrors] = scanner.getPlugins(sampleRate);

            nlohmann::json descriptions;
            nlohmann::json errors;

            for(auto const& keyJson : arguments.at("keys"))
            {
                if(!keyJson.is_object() || !keyJson.contains("identifier") || !keyJson.contains("feature"))
                {
                    return createError("Each key must be an object with 'identifier' and 'feature' fields.");
                }

                Plugin::Key key;
                key.identifier = keyJson.at("identifier").get<std::string>();
                key.feature = keyJson.at("feature").get<std::string>();
                auto const keyString = key.identifier + ":" + key.feature;
                auto const it = pluginMap.find(key);
                if(it != pluginMap.end())
                {
                    nlohmann::json description;
                    to_json(description, it->second);
                    sanitizePluginDescription(description);
                    descriptions[keyString] = description;
                }
                else
                {
                    errors[keyString] = "Plugin not found or could not be loaded";
                }
            }

            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["descriptions"] = descriptions;
            if(!errors.empty())
            {
                payload["errors"] = errors;
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
                        sanitizePluginDescription(track["description"]);
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
        if(toolName == "get_document_audio_file_layout")
        {
            auto const& documentAccessor = Instance::get().getDocumentAccessor();
            auto const layouts = documentAccessor.getAttr<Document::AttrType::reader>();

            auto layoutsJson = nlohmann::json::array();
            for(auto const& layout : layouts)
            {
                nlohmann::json item;
                item["file"] = layout.file.getFullPathName();
                item["channel"] = layout.channel;
                layoutsJson.push_back(item);
            }

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = layoutsJson.dump();
            response["content"].push_back(content);
            return response;
        }

        if(toolName == "get_audio_file_info")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("file") || !arguments.at("file").is_string())
            {
                return createError("The 'file' argument is required and must be a string.");
            }

            auto const filePath = arguments.at("file").get<std::string>();
            juce::File const file(filePath);
            if(!file.existsAsFile() || !file.hasReadAccess())
            {
                return createError("The 'file' argument must point to a readable file.");
            }

            auto& formatManager = Instance::get().getAudioFormatManager();
            auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
            if(reader == nullptr)
            {
                return createError("Failed to create audio reader for file.");
            }

            nlohmann::json payload;
            payload["file"] = file.getFullPathName().toStdString();
            payload["formatName"] = reader->getFormatName().toStdString();
            payload["sampleRate"] = reader->sampleRate;
            payload["bitsPerSample"] = reader->bitsPerSample;
            payload["usesFloatingPointData"] = reader->usesFloatingPointData;
            payload["lengthInSamples"] = static_cast<int64_t>(reader->lengthInSamples);
            if(reader->sampleRate > 0.0)
            {
                payload["durationSeconds"] = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
            }
            else
            {
                payload["durationSeconds"] = 0.0;
            }
            payload["numChannels"] = reader->numChannels;

            nlohmann::json metadata = nlohmann::json::object();
            auto const& metadataValues = reader->metadataValues;
            for(auto const& key : metadataValues.getAllKeys())
            {
                metadata[key.toStdString()] = metadataValues[key].toStdString();
            }
            payload["metadata"] = metadata;

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }

        // Document Setter Section
        if(toolName == "set_document_audio_file_layout")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("layout") || !arguments.at("layout").is_array())
            {
                return createError("The 'layout' field is required and must be an array.");
            }

            std::vector<AudioFileLayout> layouts;
            for(auto const& item : arguments.at("layout"))
            {
                if(!item.is_object() || !item.contains("file") || !item.at("file").is_string())
                {
                    return createError("Each layout item must be an object with a 'file' string field.");
                }
                if(!item.contains("channel") || !item.at("channel").is_number_integer())
                {
                    return createError("Each layout item must have a 'channel' integer field.");
                }

                auto const filePath = item.at("file").get<std::string>();
                auto const channel = item.at("channel").get<int>();
                layouts.push_back(AudioFileLayout(juce::File(filePath), channel));
            }

            auto& documentAccessor = Instance::get().getDocumentAccessor();
            documentAccessor.setAttr<Document::AttrType::reader>(layouts, NotificationType::synchronous);

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = "Audio file layout updated successfully";
            response["content"].push_back(content);
            return response;
        }

        // Document Time Zoom Section
        if(toolName == "get_document_time_zoom")
        {
            auto& documentAccessor = Instance::get().getDocumentAccessor();
            auto const& timeZoomAcsr = documentAccessor.getAcsr<Document::AcsrType::timeZoom>();

            auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();

            nlohmann::json payload;
            payload["visibleRange"]["start"] = visibleRange.getStart();
            payload["visibleRange"]["end"] = visibleRange.getEnd();
            payload["globalRange"]["start"] = globalRange.getStart();
            payload["globalRange"]["end"] = globalRange.getEnd();

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_document_time_zoom")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("start") || !arguments.at("start").is_number())
            {
                return createError("The 'start' field is required and must be a number.");
            }
            if(!arguments.contains("end") || !arguments.at("end").is_number())
            {
                return createError("The 'end' field is required and must be a number.");
            }

            auto const start = arguments.at("start").get<double>();
            auto const end = arguments.at("end").get<double>();

            if(start >= end)
            {
                return createError("The 'start' value must be less than the 'end' value.");
            }

            auto& documentAccessor = Instance::get().getDocumentAccessor();
            auto& timeZoomAcsr = documentAccessor.getAcsr<Document::AcsrType::timeZoom>();

            timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(juce::Range<double>(start, end), NotificationType::synchronous);

            auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            nlohmann::json payload;
            payload["visibleRange"]["start"] = visibleRange.getStart();
            payload["visibleRange"]["end"] = visibleRange.getEnd();

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
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
                    groupAcsr.setAttr<Group::AttrType::name>(name, NotificationType::synchronous);
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
                    groupAcsr.setAttr<Group::AttrType::layout>(layout, NotificationType::synchronous);
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
        if(toolName == "get_track_graphics")
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
            nlohmann::json graphics;
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
                    auto const& settings = trackAcsr.getAttr<Track::AttrType::graphicsSettings>();
                    nlohmann::json trackGraphics;
                    if(frameType.has_value() && frameType.value() == Track::FrameType::vector)
                    {
                        switch(frameType.value())
                        {
                            case Track::FrameType::label:
                            {
                                trackGraphics["colorBackground"] = colourToString(settings.colours.background);
                                trackGraphics["colorForeground"] = colourToString(settings.colours.foreground);
                                trackGraphics["colorDuration"] = colourToString(settings.colours.duration);
                                trackGraphics["colorText"] = colourToString(settings.colours.text);
                                trackGraphics["colorShadow"] = colourToString(settings.colours.shadow);

                                trackGraphics["fontName"] = settings.font.getName();
                                trackGraphics["fontSize"] = settings.font.getHeight();
                                trackGraphics["fontStyle"] = settings.font.getStyle();
                                trackGraphics["lineWidth"] = settings.lineWidth;

                                trackGraphics["labelLayoutPosition"] = settings.labelLayout.position;
                                trackGraphics["labelLayoutJustification"] = magic_enum::enum_name(settings.labelLayout.justification);
                                break;
                            }
                            case Track::FrameType::value:
                            {
                                trackGraphics["colorBackground"] = colourToString(settings.colours.background);
                                trackGraphics["colorForeground"] = colourToString(settings.colours.foreground);
                                trackGraphics["colorText"] = colourToString(settings.colours.text);
                                trackGraphics["colorShadow"] = colourToString(settings.colours.shadow);

                                trackGraphics["fontName"] = settings.font.getName();
                                trackGraphics["fontSize"] = settings.font.getHeight();
                                trackGraphics["fontStyle"] = settings.font.getStyle();
                                trackGraphics["lineWidth"] = settings.lineWidth;

                                if(settings.unit.has_value())
                                {
                                    trackGraphics["unit"] = settings.unit.value();
                                }
                                break;
                            }
                            case Track::FrameType::vector:
                            {
                                trackGraphics["colorMap"] = magic_enum::enum_name(settings.colours.map);

                                if(settings.unit.has_value())
                                {
                                    trackGraphics["unit"] = settings.unit.value();
                                }
                                break;
                            }
                        }
                    }
                    graphics[identifier] = trackGraphics;
                }
                else
                {
                    response["isError"] = true;
                    graphics[identifier] = "The track doesn't exist.";
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = graphics.dump();
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
                    sanitizePluginDescription(description);
                    descriptions[identifier] = description;
                }
                else
                {
                    response["isError"] = true;
                    descriptions[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
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
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = parameters.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_results")
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
            nlohmann::json results;
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                auto const identifier = identifierJson.get<std::string>();
                if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    response["isError"] = true;
                    results[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                    continue;
                }

                auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                auto const frameType = Track::Tools::getFrameType(trackAcsr);
                if(!frameType.has_value())
                {
                    response["isError"] = true;
                    results[identifier] = "The track type is unknown. The analysis might be runnning.";
                }
                else if(frameType.value() == Track::FrameType::vector)
                {
                    results[identifier]["message"] = "Vector track results are not supported by this tool.";
                }
                else
                {

                    auto const& trackResults = trackAcsr.getAttr<Track::AttrType::results>();
                    auto const access = trackResults.getReadAccess();
                    if(!static_cast<bool>(access))
                    {
                        response["isError"] = true;
                        results[identifier] = "The result data are currently locked.";
                    }
                    else
                    {
                        if(auto const markers = trackResults.getMarkers())
                        {
                            auto channels = nlohmann::json::array();
                            for(auto const& channelMarkers : *markers)
                            {
                                auto channel = nlohmann::json::array();
                                for(auto const& marker : channelMarkers)
                                {
                                    nlohmann::json frame;
                                    frame["time"] = std::get<0_z>(marker);
                                    frame["duration"] = std::get<1_z>(marker);
                                    frame["label"] = std::get<2_z>(marker);
                                    if(!std::get<3_z>(marker).empty())
                                    {
                                        frame["extra"] = std::get<3_z>(marker);
                                    }
                                    channel.push_back(std::move(frame));
                                }
                                channels.push_back(std::move(channel));
                            }
                            results[identifier]["channels"] = std::move(channels);
                        }
                        else if(auto const points = trackResults.getPoints())
                        {
                            auto channels = nlohmann::json::array();
                            for(auto const& channelPoints : *points)
                            {
                                auto channel = nlohmann::json::array();
                                for(auto const& point : channelPoints)
                                {
                                    nlohmann::json frame;
                                    frame["time"] = std::get<0_z>(point);
                                    frame["duration"] = std::get<1_z>(point);
                                    if(std::get<2_z>(point).has_value())
                                    {
                                        frame["value"] = std::get<2_z>(point).value();
                                    }
                                    else
                                    {
                                        frame["value"] = {};
                                    }
                                    if(!std::get<3_z>(point).empty())
                                    {
                                        frame["extra"] = std::get<3_z>(point);
                                    }
                                    channel.push_back(std::move(frame));
                                }
                                channels.push_back(std::move(channel));
                            }
                            results[identifier]["channels"] = std::move(channels);
                        }
                        else
                        {
                            response["isError"] = true;
                            results[identifier] = "No results are not available for this track.";
                        }
                    }
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.dump();
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
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = inputTracks.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_is_visible_in_group")
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
            nlohmann::json visibilityByTrack;
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
                    visibilityByTrack[identifier] = trackAcsr.getAttr<Track::AttrType::showInGroup>();
                }
                else
                {
                    response["isError"] = true;
                    visibilityByTrack[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                }
            }
            nlohmann::json payload;
            payload["visibleInGroup"] = visibilityByTrack;
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
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
        if(toolName == "get_track_extra_thresholds")
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
            nlohmann::json thresholdsByTrack;
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
                    auto const& thresholds = trackAcsr.getAttr<Track::AttrType::extraThresholds>();
                    auto& jsonThresholds = thresholdsByTrack[identifier] = nlohmann::json::array();
                    for(auto const& threshold : thresholds)
                    {
                        if(threshold.has_value())
                        {
                            jsonThresholds.push_back(threshold.value());
                        }
                        else
                        {
                            jsonThresholds.push_back(nullptr);
                        }
                    }
                }
                else
                {
                    response["isError"] = true;
                    thresholdsByTrack[identifier] = juce::String("The track \"TRACKID\" doesn't exist.").replace("TRACKID", identifier);
                }
            }
            nlohmann::json payload;
            payload["extraThresholds"] = thresholdsByTrack;
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }

        // Track Setter Section
        if(toolName == "set_track_graphics")
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
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    auto const frameType = Track::Tools::getFrameType(trackAcsr);
                    auto settings = trackAcsr.getAttr<Track::AttrType::graphicsSettings>();
                    auto const isLabel = frameType.has_value() || frameType.value() == Track::FrameType::label;
                    auto const isValue = frameType.has_value() || frameType.value() == Track::FrameType::value;
                    auto const isVector = frameType.has_value() || frameType.value() == Track::FrameType::vector;

                    // Colour map (vector tracks only)
                    if(trackJson.contains("colorMap"))
                    {
                        if(!trackJson.at("colorMap").is_string())
                        {
                            return createError("The 'colorMap' field must be a string.");
                        }
                        auto const colorMapStr = trackJson.at("colorMap").get<std::string>();
                        auto const colorMap = magic_enum::enum_cast<Track::ColourMap>(colorMapStr);
                        if(!colorMap.has_value())
                        {
                            response["isError"] = true;
                            results.add(juce::String("The colour map 'COLORMAPNAME' doesn't exist. The value can be 'Parula', 'Heat', 'Jet', 'Turbo', 'Hot', 'Gray', 'Magma', 'Inferno', 'Plasma', 'Viridis', 'Cividis', or 'Github'").replace("COLORMAPNAME", colorMapStr));
                            break;
                        }
                        else if(isVector)
                        {
                            settings.colours.map = colorMap.value();
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String("The track \"TRACKID\" doesn't support the \"colorMap\" property.").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    auto const applyColour = [&](const char* fieldName, auto& refValue, bool isCompatible)
                    {
                        auto const colourStr = trackJson.at(fieldName).get<std::string>();
                        auto const colour = stringToColour(colourStr);
                        if(!colour.has_value())
                        {
                            response["isError"] = true;
                            results.add(juce::String("The colour code 'COLORCODE' is invalid and must be in the format #RRGGBBAA.").replace("COLORCODE", colourStr));
                            return false;
                        }
                        if(isCompatible)
                        {
                            refValue = colour.value();
                            return true;
                        }
                        response["isError"] = true;
                        results.add(juce::String("The track \"TRACKID\" doesn't support the \"FIELDNAME\" property.").replace("TRACKID", identifier).replace("FIELDNAME", fieldName));
                        return false;
                    };

                    // Colours (label and value tracks)
                    if(trackJson.contains("colorBackground"))
                    {
                        if(!trackJson.at("colorBackground").is_string())
                        {
                            return createError("The 'colorBackground' field must be a string.");
                        }
                        if(!applyColour("colorBackground", settings.colours.background, isLabel || isValue))
                        {
                            break;
                        }
                    }
                    if(trackJson.contains("colorForeground"))
                    {
                        if(!trackJson.at("colorForeground").is_string())
                        {
                            return createError("The 'colorForeground' field must be a string.");
                        }
                        if(!applyColour("colorForeground", settings.colours.foreground, isLabel || isValue))
                        {
                            break;
                        }
                    }
                    if(trackJson.contains("colorDuration"))
                    {
                        if(!trackJson.at("colorDuration").is_string())
                        {
                            return createError("The 'colorDuration' field must be a string.");
                        }
                        if(!applyColour("colorDuration", settings.colours.duration, isLabel))
                        {
                            break;
                        }
                    }
                    if(trackJson.contains("colorText"))
                    {
                        if(!trackJson.at("colorText").is_string())
                        {
                            return createError("The 'colorText' field must be a string.");
                        }
                        if(!applyColour("colorText", settings.colours.text, isLabel))
                        {
                            break;
                        }
                    }
                    if(trackJson.contains("colorShadow"))
                    {
                        if(!trackJson.at("colorShadow").is_string())
                        {
                            return createError("The 'colorShadow' field must be a string.");
                        }
                        if(!applyColour("colorShadow", settings.colours.shadow, isLabel))
                        {
                            break;
                        }
                    }

                    // Font (label and value tracks)
                    if(trackJson.contains("fontName") || trackJson.contains("fontSize") || trackJson.contains("fontStyle"))
                    {
                        auto fontName = settings.font.getName();
                        auto fontStyle = settings.font.getStyle();
                        auto fontSize = settings.font.getHeight();

                        if(trackJson.contains("fontName"))
                        {
                            if(!trackJson.at("fontName").is_string())
                            {
                                return createError("The 'fontName' field must be a string.");
                            }
                            fontName = juce::String(trackJson.at("fontName").get<std::string>());
                        }
                        if(trackJson.contains("fontStyle"))
                        {
                            if(!trackJson.at("fontStyle").is_string())
                            {
                                return createError("The 'fontStyle' field must be a string.");
                            }
                            fontStyle = juce::String(trackJson.at("fontStyle").get<std::string>());
                        }
                        if(trackJson.contains("fontSize"))
                        {
                            if(!trackJson.at("fontSize").is_number())
                            {
                                return createError("The 'fontSize' field must be a floating point number.");
                            }
                            fontSize = trackJson.at("fontSize").get<float>();
                        }

                        if(isLabel || isValue)
                        {
                            settings.font = juce::FontOptions(fontName, fontStyle, fontSize);
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String("The track \"TRACKID\" doesn't support the fonts properties.").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Line width (label and value tracks)
                    if(trackJson.contains("lineWidth"))
                    {
                        if(!trackJson.at("lineWidth").is_number())
                        {
                            return createError("The 'lineWidth' field must be a floating point number.");
                        }
                        if(isLabel || isValue)
                        {
                            settings.lineWidth = trackJson.at("lineWidth").get<float>();
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String("The track \"TRACKID\" doesn't support the lineWidth property.").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Unit (value and vector tracks)
                    if(trackJson.contains("unit"))
                    {
                        if(!trackJson.at("unit").is_string())
                        {
                            return createError("The 'unit' field must be a string.");
                        }
                        if(isValue || isValue)
                        {
                            auto const unitStr = juce::String(trackJson.at("unit").get<std::string>());
                            settings.unit = unitStr.isEmpty() ? std::nullopt : std::optional<juce::String>(unitStr);
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String("The track \"TRACKID\" doesn't support the unit property.").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Label layout (label tracks only)
                    if(trackJson.contains("labelLayoutPosition") || trackJson.contains("labelLayoutJustification"))
                    {
                        auto position = settings.labelLayout.position;
                        auto justification = settings.labelLayout.justification;
                        if(trackJson.contains("labelLayoutPosition"))
                        {
                            if(!trackJson.at("labelLayoutPosition").is_number())
                            {
                                return createError("The 'labelLayoutPosition' field must be a floating point number.");
                            }
                            position = trackJson.at("labelLayoutPosition").get<float>();
                        }
                        if(trackJson.contains("labelLayoutJustification"))
                        {
                            if(!trackJson.at("labelLayoutJustification").is_string())
                            {
                                return createError("The 'labelLayoutJustification' field must be a string.");
                            }
                            auto const justificationStr = trackJson.at("labelLayoutJustification").get<std::string>();
                            auto const justificationOpt = magic_enum::enum_cast<Track::LabelLayout::Justification>(justificationStr);
                            if(!justificationOpt.has_value())
                            {
                                response["isError"] = true;
                                results.add(juce::String("The label layout justification 'LABELLAYOUTJUST' doesn't exist. The value can be 'top', 'centred', or 'bottom'").replace("LABELLAYOUTJUST", justificationStr));
                                break;
                            }
                            justification = justificationOpt.value();
                        }
                        if(isLabel)
                        {
                            settings.labelLayout.position = position;
                            settings.labelLayout.justification = justification;
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String("The track \"TRACKID\" doesn't support the label layout properties.").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    trackAcsr.setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
                    results.add(juce::String("The graphics settings of track \"TRACKID\" have been updated.").replace("TRACKID", identifier));
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
                documentDir.endAction(ActionState::newTransaction, juce::translate("Change track graphics (Neuralyzer)"));
            }
            else
            {
                documentDir.endAction(ActionState::abort);
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n").toStdString();
            response["content"].push_back(content);
            return response;
        }
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
                    trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
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
                        trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
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
                    trackAcsr.setAttr<Track::AttrType::input>(input, NotificationType::synchronous);
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
        if(toolName == "set_track_is_visible_in_group")
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
                if(!trackJson.contains("visibleInGroup") || !trackJson.at("visibleInGroup").is_boolean())
                {
                    return createError("The 'visibleInGroup' field is required and must be a boolean.");
                }

                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const visibleInGroup = trackJson.at("visibleInGroup").get<bool>();

                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    trackAcsr.setAttr<Track::AttrType::showInGroup>(visibleInGroup, NotificationType::synchronous);
                    results.add(juce::String("The track \"TRACKID\" visibility in group has been set to \"STATE\".").replace("TRACKID", identifier).replace("STATE", visibleInGroup ? "true" : "false"));
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
                documentDir.endAction(ActionState::newTransaction, juce::translate("Set track visibility in group (Neuralyzer)"));
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
        if(toolName == "set_track_extra_thresholds")
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
                if(!trackJson.contains("extraThresholds") || !trackJson.at("extraThresholds").is_array())
                {
                    return createError("The 'extraThresholds' field is required and must be an array.");
                }

                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const& thresholdsArray = trackJson.at("extraThresholds");

                std::vector<std::optional<float>> newThresholds;
                for(auto const& value : thresholdsArray)
                {
                    if(value.is_null())
                    {
                        newThresholds.push_back(std::nullopt);
                    }
                    else if(value.is_number())
                    {
                        newThresholds.push_back(value.get<float>());
                    }
                    else
                    {
                        return createError("Each threshold value must be a number or null.");
                    }
                }

                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    trackAcsr.setAttr<Track::AttrType::extraThresholds>(newThresholds, NotificationType::synchronous);
                    results.add(juce::String("The extra thresholds of track \"TRACKID\" have been updated.").replace("TRACKID", identifier));
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
                documentDir.endAction(ActionState::newTransaction, juce::translate("Set track extra thresholds (Neuralyzer)"));
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

        // Group Creation/Removal Section
        if(toolName == "create_groups")
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
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            juce::StringArray results;
            nlohmann::json created = nlohmann::json::array();
            for(auto const& groupJson : groups)
            {
                if(!groupJson.is_object())
                {
                    return createError("The 'groups' argument is required and must be an array of objects.");
                }
                if(groupJson.contains("name") && !groupJson.at("name").is_string())
                {
                    return createError("The 'name' field is required and must be a string.");
                }
                auto const name = groupJson.value("name", juce::String{});
                auto const position = groupJson.value("position", 0_z);
                auto const [result, identifier] = documentDir.addGroup(position, NotificationType::synchronous);
                if(result.failed())
                {
                    response["isError"] = true;
                    results.add(juce::String("Failed to create group \"NAME\": REASON.").replace("NAME", name).replace("REASON", result.getErrorMessage()));
                    break;
                }
                auto& documentAcsr = Instance::get().getDocumentAccessor();
                auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                if(!name.isEmpty())
                {
                    groupAcsr.setAttr<Group::AttrType::name>(name, NotificationType::synchronous);
                }
                created.push_back(identifier);
                results.add(juce::String("Created group \"GROUPID\" with name \"NAME\".").replace("GROUPID", identifier).replace("NAME", groupAcsr.getAttr<Group::AttrType::name>()));
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Create groups (Neuralyzer)"));
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
            return response;
        }
        if(toolName == "remove_groups")
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
                if(Document::Tools::hasGroupAcsr(documentAcsr, identifier))
                {
                    auto const result = documentDir.removeGroup(identifier, NotificationType::synchronous);
                    if(result.failed())
                    {
                        response["isError"] = true;
                        results.add(juce::String("The group \"GROUPID\" could not be removed: REASON.").replace("GROUPID", identifier).replace("REASON", result.getErrorMessage()));
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
                    results.add(juce::String("The group \"GROUPID\" doesn't exist.").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Remove groups (Neuralyzer)"));
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

    class ColourParsingUnitTest
    : public juce::UnitTest
    {
    public:
        ColourParsingUnitTest()
        : juce::UnitTest("ColourParsing", "Application")
        {
        }

        ~ColourParsingUnitTest() override = default;

        void runTest() override
        {
            beginTest("colourToString");
            {
                expectEquals(colourToString(juce::Colour::fromRGBA(0, 0, 0, 0)), std::string{"#00000000"});
                expectEquals(colourToString(juce::Colour::fromRGBA(0, 0, 0, 255)), std::string{"#000000ff"});
                expectEquals(colourToString(juce::Colour::fromRGBA(255, 255, 255, 255)), std::string{"#ffffffff"});
                expectEquals(colourToString(juce::Colour::fromRGBA(255, 0, 0, 255)), std::string{"#ff0000ff"});
                expectEquals(colourToString(juce::Colour::fromRGBA(0xAA, 0xBB, 0xCC, 0xDD)), std::string{"#aabbccdd"});
            }

            beginTest("stringToColour");
            {
                expect(!stringToColour("invalid").has_value());
                expect(!stringToColour("ff0000ff").has_value());

                // "#RRGGBB" format: alpha defaults to 255
                auto const red6 = stringToColour("#ff0000");
                expect(red6.has_value());
                if(red6.has_value())
                {
                    expectEquals(red6->getRed(), static_cast<uint8_t>(255));
                    expectEquals(red6->getGreen(), static_cast<uint8_t>(0));
                    expectEquals(red6->getBlue(), static_cast<uint8_t>(0));
                    expectEquals(red6->getAlpha(), static_cast<uint8_t>(255));
                }

                // "#RRGGBBAA" format
                auto const red8 = stringToColour("#ff000080");
                expect(red8.has_value());
                if(red8.has_value())
                {
                    expectEquals(red8->getRed(), static_cast<uint8_t>(255));
                    expectEquals(red8->getGreen(), static_cast<uint8_t>(0));
                    expectEquals(red8->getBlue(), static_cast<uint8_t>(0));
                    expectEquals(red8->getAlpha(), static_cast<uint8_t>(128));
                }

                // Strings with surrounding quotes should also be accepted
                auto const quoted = stringToColour("\"#aabbccdd\"");
                expect(quoted.has_value());
                if(quoted.has_value())
                {
                    expectEquals(quoted->getRed(), static_cast<uint8_t>(0xAA));
                    expectEquals(quoted->getGreen(), static_cast<uint8_t>(0xBB));
                    expectEquals(quoted->getBlue(), static_cast<uint8_t>(0xCC));
                    expectEquals(quoted->getAlpha(), static_cast<uint8_t>(0xDD));
                }
            }

            beginTest("round trip");
            {
                auto const roundTrip = [&](juce::Colour const& colour)
                {
                    auto const str = colourToString(colour);
                    auto const result = stringToColour(str);
                    expect(result.has_value());
                    if(result.has_value())
                    {
                        expectEquals(result->getRed(), colour.getRed());
                        expectEquals(result->getGreen(), colour.getGreen());
                        expectEquals(result->getBlue(), colour.getBlue());
                        expectEquals(result->getAlpha(), colour.getAlpha());
                    }
                };
                roundTrip(juce::Colour::fromRGBA(0, 0, 0, 0));
                roundTrip(juce::Colour::fromRGBA(0, 0, 0, 255));
                roundTrip(juce::Colour::fromRGBA(255, 255, 255, 255));
                roundTrip(juce::Colour::fromRGBA(255, 0, 0, 128));
                roundTrip(juce::Colour::fromRGBA(0xAA, 0xBB, 0xCC, 0xDD));
            }
        }
    };

    static ColourParsingUnitTest colourParsingUnitTest;
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
        auto list = nlohmann::json::parse(AnlNeuralyzerData::tools_list_json);
        // Remove outputSchema from list
        for(auto& tool : list["tools"])
        {
            tool.erase("outputSchema");
        }
        return list;
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
                result["protocolVersion"] = "2025-11-25";
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
