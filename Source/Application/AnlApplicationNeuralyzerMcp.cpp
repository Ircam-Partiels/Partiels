#include "AnlApplicationNeuralyzerMcp.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackLoader.h"
#include "../Track/AnlTrackTools.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"
#include <AnlNeuralyzerData.h>
#include <AnlNeuralyzerTutorialData.h>

ANALYSE_FILE_BEGIN

nlohmann::json Application::Neuralyzer::Mcp::createError(juce::String const& what)
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
    template <typename Predicate>
    static bool waitUntil(int waitSeconds, Predicate&& predicate)
    {
        if(predicate())
        {
            return true;
        }
        if(waitSeconds <= 0)
        {
            return false;
        }
        auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(waitSeconds);
        while(std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if(predicate())
            {
                return true;
            }
        }
        return predicate();
    }

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
            resource["title"] = AnlNeuralyzerTutorialData::originalFilenames[i];
            resource["description"] = AnlNeuralyzerTutorialData::originalFilenames[i];
            resource["mimeType"] = "text/markdown";
            result["resources"].push_back(std::move(resource));
        }
        return result;
    }

    static nlohmann::json readResource(nlohmann::json const& request)
    {
        if(!request.contains("params") || !request.at("params").is_object())
        {
            return createError("The 'params' field is required and must be an object.");
        }

        auto const& params = request.at("params");
        if(!params.contains("uri") || !params.at("uri").is_string())
        {
            return createError("The 'uri' field is required and must be a string.");
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
            auto& scanner = Instance::get().getPluginListScanner();
            auto const [pluginMap, errors] = scanner.getPlugins(48000.0, false);
            nlohmann::json plugins;
            to_json(plugins, pluginMap);
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

        nlohmann::json response;
        response["jsonrpc"] = jsonrpc;
        auto& error = response["error"];
        error["code"] = -32002;
        error["message"] = "Resource not found";
        error["data"]["uri"] = uri;
        return response;
    }

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

    static nlohmann::json zoomPayload(Zoom::Accessor const& zoomAcsr)
    {
        nlohmann::json payload;
        auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        payload["visibleRange"]["start"] = visibleRange.getStart();
        payload["visibleRange"]["end"] = visibleRange.getEnd();
        payload["globalRange"]["start"] = globalRange.getStart();
        payload["globalRange"]["end"] = globalRange.getEnd();
        return payload;
    }

    static nlohmann::json statisticsPayload(std::vector<float> const& values)
    {
        nlohmann::json payload;
        if(values.empty())
        {
            payload["min"] = nullptr;
            payload["max"] = nullptr;
            payload["median"] = nullptr;
            payload["mean"] = nullptr;
            payload["variance"] = nullptr;
            payload["skewness"] = nullptr;
            return payload;
        }

        // Min & max
        auto const [minIt, maxIt] = std::minmax_element(values.cbegin(), values.cend());
        auto const minValue = *minIt;
        auto const maxValue = *maxIt;

        // Median
        auto copy = values;
        std::sort(copy.begin(), copy.end());
        auto const mid = copy.size() / 2_z;
        auto const median = copy.size() % 2_z == 0_z ? (copy.at(mid - 1_z) + copy.at(mid)) / 2.0 : copy.at(mid);

        // Mean
        auto const count = static_cast<long double>(values.size());
        auto const sum = std::accumulate(values.cbegin(), values.cend(), 0.0L);
        auto const mean = sum / count;

        // Variance and skewness
        auto variance = 0.0L;
        auto skewness = 0.0L;
        for(auto const& value : values)
        {
            auto const centered = static_cast<long double>(value) - mean;
            auto const power = centered * centered;
            variance += power;
            skewness += power * centered;
        }
        variance /= count;
        skewness /= count;
        auto const stdDeviation = std::sqrt(variance);
        skewness = stdDeviation > 0.0L ? static_cast<double>(skewness / (stdDeviation * stdDeviation * stdDeviation)) : 0.0L;

        payload["min"] = minValue;
        payload["max"] = maxValue;
        payload["median"] = median;
        payload["mean"] = static_cast<double>(mean);
        payload["variance"] = static_cast<double>(variance);
        payload["skewness"] = static_cast<double>(skewness);
        return payload;
    }

    static std::variant<std::vector<juce::Range<double>>, nlohmann::json> getTimeRangesFromArguments(nlohmann::json const& arguments)
    {
        std::vector<juce::Range<double>> timeRanges;
        if(arguments.contains("timeRanges"))
        {
            if(!arguments.at("timeRanges").is_array())
            {
                return createError("The 'timeRanges' field must be an array of time ranges [[startTime, endTime], ...].");
            }
            auto const& timeRangesArray = arguments.at("timeRanges");
            for(auto const& timeRangeArray : timeRangesArray)
            {
                if(!timeRangeArray.is_array())
                {
                    return createError("Each entry of 'timeRanges' must be an array of two numbers [startTime, endTime].");
                }
                if(timeRangeArray.size() != 2)
                {
                    return createError("Each entry of 'timeRanges' must have exactly 2 elements [startTime, endTime].");
                }
                if(!timeRangeArray[0].is_number() || !timeRangeArray[1].is_number())
                {
                    return createError("The 'timeRanges' elements must be numbers.");
                }
                auto const startTime = timeRangeArray[0].get<double>();
                auto const endTime = timeRangeArray[1].get<double>();
                if(startTime > endTime)
                {
                    return createError("Each 'timeRanges' startTime must be less than or equal to endTime.");
                }
                timeRanges.emplace_back(startTime, endTime);
            }
        }
        return timeRanges;
    }

    static std::variant<std::set<size_t>, nlohmann::json> getChannelsFromArguments(nlohmann::json const& arguments)
    {
        std::set<size_t> channels;
        if(arguments.contains("channels"))
        {
            if(!arguments.at("channels").is_array())
            {
                return createError("The 'channels' field must be an array of non-negative integers.");
            }
            for(auto const& channelJson : arguments.at("channels"))
            {
                if(!channelJson.is_number_integer())
                {
                    return createError("The 'channels' field must be an array of non-negative integers.");
                }
                auto const channel = channelJson.get<int64_t>();
                if(channel < 0)
                {
                    return createError("The 'channels' field must be an array of non-negative integers.");
                }
                channels.insert(static_cast<size_t>(channel));
            }
        }
        return channels;
    }

    class ScopedDocumentAction
    {
    public:
        explicit ScopedDocumentAction(Document::Director& documentDir)
        : mDocumentDir(documentDir)
        {
            MiscWeakAssert(!mDocumentDir.isPerformingAction());
            mDocumentDir.startAction();
        }

        ~ScopedDocumentAction()
        {
            if(mDocumentDir.isPerformingAction())
            {
                mDocumentDir.endAction(ActionState::abort);
            }
        }

        void commit(juce::String const& actionName)
        {
            MiscWeakAssert(mDocumentDir.isPerformingAction());
            mDocumentDir.endAction(ActionState::newTransaction, actionName);
        }

        void abort()
        {
            MiscWeakAssert(mDocumentDir.isPerformingAction());
            mDocumentDir.endAction(ActionState::abort);
        }

    private:
        Document::Director& mDocumentDir;
    };

    static nlohmann::json performToolsCall(nlohmann::json const& methodParams)
    {
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
            auto const [pluginMap, errors] = scanner.getPlugins(sampleRate, false);
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
                plugin.back().erase("copyright");
                plugin.back().erase("inputDomain");
                plugin.back().erase("version");
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
            auto const [pluginMap, scanErrors] = scanner.getPlugins(sampleRate, false);

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
                group["name"] = groupAcsr.getAttr<Group::AttrType::name>();
                auto const& groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                nlohmann::json tracks = nlohmann::json::array();
                for(auto const& trackIdentifier : groupLayout)
                {
                    nlohmann::json track;
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackIdentifier);
                    track["identifier"] = trackIdentifier;
                    track["name"] = trackAcsr.getAttr<Track::AttrType::name>();
                    if(deep)
                    {
                        to_json(track["description"], trackAcsr.getAttr<Track::AttrType::description>());
                        sanitizePluginDescription(track["description"]);
                        to_json(track["parameters"], trackAcsr.getAttr<Track::AttrType::state>().parameters);
                        if(Track::Tools::supportsInputTracks(trackAcsr))
                        {
                            track["inputs"] = trackAcsr.getAttr<Track::AttrType::inputs>();
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
                        else
                        {
                            response["isError"] = true;
                            track["type"] = "Unknown track type, cannot determine the type of value. Either the track is invalid or the analysis is still running.";
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
            payload["file"] = file.getFullPathName();
            payload["formatName"] = reader->getFormatName();
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

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(timeZoomAcsr).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_vertical_zoom")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
            }

            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
            auto const frameType = Track::Tools::getFrameType(trackAcsr);
            if(!frameType.has_value() || frameType.value() == Track::FrameType::label)
            {
                return createError("The track vertical zoom is only available for point and vector tracks.");
            }

            auto zoomAcsr = Track::Tools::getVerticalZoomAccessor(trackAcsr, false);
            if(!zoomAcsr.has_value())
            {
                return createError("The requested zoom accessor is not available.");
            }

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(zoomAcsr.value().get()).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_group_vertical_zoom")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
            }

            auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
            auto const trackAcsr = Group::Tools::getVerticalZoomTrackAcsr(groupAcsr);
            if(!trackAcsr.has_value())
            {
                return createError("The group has no point or vector track with a vertical zoom.");
            }

            auto zoomAcsr = Track::Tools::getVerticalZoomAccessor(trackAcsr.value().get(), true);
            if(!zoomAcsr.has_value())
            {
                return createError("The requested zoom accessor is not available.");
            }

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(zoomAcsr.value().get()).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_time_selection")
        {
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
            auto const timeSelection = transportAcsr.getAttr<Transport::AttrType::selection>();

            nlohmann::json payload;
            payload["isEmpty"] = timeSelection.isEmpty();
            payload["start"] = timeSelection.getStart();
            payload["end"] = timeSelection.getEnd();

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_selected_items")
        {
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            auto const selectedGroups = Document::Selection::getGroups(documentAcsr);
            auto const selectedTracks = Document::Selection::getTracks(documentAcsr);

            nlohmann::json groups = nlohmann::json::array();
            for(auto const& id : selectedGroups)
            {
                groups.push_back(id);
            }
            nlohmann::json tracks = nlohmann::json::array();
            for(auto const& id : selectedTracks)
            {
                tracks.push_back(id);
            }

            nlohmann::json payload;
            payload["groups"] = groups;
            payload["tracks"] = tracks;

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
                    names[identifier] = juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier);
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
                    layouts[identifier] = juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier);
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
            ScopedDocumentAction actionGuard(documentDir);
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
                    results.add(juce::String(R"(The group "GROUPID" has been renamed "NAME".)").replace("GROUPID", identifier).replace("NAME", name));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Change group name (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            ScopedDocumentAction actionGuard(documentDir);
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
                    results.add(juce::String(R"(The group "GROUPID" layout has been modified.)").replace("GROUPID", identifier));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Change group layout (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
                        response["isError"] = true;
                        types["error"] = "Unknown track type, cannot determine the type of value. Either the track is invalid or the analysis is still running.";
                    }
                }
                else
                {
                    response["isError"] = true;
                    types[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
                    if(frameType.has_value())
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
                    else
                    {
                        response["isError"] = true;
                        trackGraphics["error"] = "Unknown track type, cannot determine graphics settings.";
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
        if(toolName == "set_track_vertical_zoom")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }
            if(!arguments.contains("start") || !arguments.at("start").is_number() || !arguments.contains("end") || !arguments.at("end").is_number())
            {
                return createError("The 'start' and 'end' arguments are required and must be numbers.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto const start = arguments.at("start").get<double>();
            auto const end = arguments.at("end").get<double>();
            if(start >= end)
            {
                return createError("The 'start' value must be less than the 'end' value.");
            }

            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
            }

            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
            auto const frameType = Track::Tools::getFrameType(trackAcsr);
            if(!frameType.has_value() || frameType.value() == Track::FrameType::label)
            {
                return createError("The track vertical zoom is only available for point and vector tracks.");
            }

            auto zoomAcsr = Track::Tools::getVerticalZoomAccessor(trackAcsr, true);
            if(!zoomAcsr.has_value())
            {
                return createError("The requested zoom accessor is not available.");
            }

            zoomAcsr.value().get().setAttr<Zoom::AttrType::visibleRange>(juce::Range<double>(start, end), NotificationType::synchronous);

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(zoomAcsr.value().get()).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_group_vertical_zoom")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }
            if(!arguments.contains("start") || !arguments.at("start").is_number() || !arguments.contains("end") || !arguments.at("end").is_number())
            {
                return createError("The 'start' and 'end' arguments are required and must be numbers.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto const start = arguments.at("start").get<double>();
            auto const end = arguments.at("end").get<double>();
            if(start >= end)
            {
                return createError("The 'start' value must be less than the 'end' value.");
            }

            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasGroupAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
            }

            auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
            auto const trackAcsr = Group::Tools::getVerticalZoomTrackAcsr(groupAcsr);
            if(!trackAcsr.has_value())
            {
                return createError("The group has no point or vector track with a vertical zoom.");
            }

            auto zoomAcsr = Track::Tools::getVerticalZoomAccessor(trackAcsr.value().get(), true);
            if(!zoomAcsr.has_value())
            {
                return createError("The requested zoom accessor is not available.");
            }

            zoomAcsr.value().get().setAttr<Zoom::AttrType::visibleRange>(juce::Range<double>(start, end), NotificationType::synchronous);

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(zoomAcsr.value().get()).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_value_range")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
            }

            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
            auto const frameType = Track::Tools::getFrameType(trackAcsr);
            if(!frameType.has_value() || frameType.value() != Track::FrameType::vector)
            {
                return createError("The track value range is only available for vector tracks.");
            }

            auto const& valueZoomAcsr = trackAcsr.getAcsr<Track::AcsrType::valueZoom>();
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(valueZoomAcsr).dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "set_track_value_range")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' argument is required and must be a string.");
            }
            if(!arguments.contains("start") || !arguments.at("start").is_number() || !arguments.contains("end") || !arguments.at("end").is_number())
            {
                return createError("The 'start' and 'end' arguments are required and must be numbers.");
            }

            auto const identifier = arguments.at("identifier").get<std::string>();
            auto const start = arguments.at("start").get<double>();
            auto const end = arguments.at("end").get<double>();
            if(start >= end)
            {
                return createError("The 'start' value must be less than the 'end' value.");
            }

            auto& documentAcsr = Instance::get().getDocumentAccessor();
            if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                return createError(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
            }

            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
            auto const frameType = Track::Tools::getFrameType(trackAcsr);
            if(!frameType.has_value() || frameType.value() != Track::FrameType::vector)
            {
                return createError("The track value range is only available for vector tracks.");
            }

            auto const pluginRange = Track::Tools::getValueRange(trackAcsr.getAttr<Track::AttrType::description>());
            auto const resultsRange = Track::Tools::getResultRange(trackAcsr);
            auto& valueZoomAcsr = trackAcsr.getAcsr<Track::AcsrType::valueZoom>();
            auto const range = juce::Range<double>(start, end);
            if(pluginRange.has_value() && range == pluginRange.value())
            {
                trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::plugin, NotificationType::synchronous);
            }
            else if(resultsRange.has_value() && range == resultsRange.value())
            {
                trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::results, NotificationType::synchronous);
            }
            else
            {
                trackAcsr.setAttr<Track::AttrType::zoomValueMode>(Track::ZoomValueMode::custom, NotificationType::synchronous);
            }
            valueZoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = zoomPayload(valueZoomAcsr).dump();
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
                    names[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
                    descriptions[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
                    parameters[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
                }
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = parameters.dump();
            response["content"].push_back(content);
            return response;
        }
        if(toolName == "get_track_results_raw")
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
            auto timeRangesResults = getTimeRangesFromArguments(arguments);
            if(timeRangesResults.index() == 1)
            {
                return std::get<1>(timeRangesResults);
            }
            auto const timeRanges = std::get<0>(timeRangesResults);

            auto channelResult = getChannelsFromArguments(arguments);
            if(channelResult.index() == 1)
            {
                return std::get<1>(channelResult);
            }
            auto const channels = std::get<0>(channelResult);

            if(arguments.contains("header") && !arguments.at("header").is_string())
            {
                return createError("The 'header' field must be one of these strings: none, standard, or generic.");
            }
            using CsvHeaderType = Track::Exporter::CsvHeaderType;
            auto const header = magic_enum::enum_cast<CsvHeaderType>(arguments.value("header", "none"));
            if(!header.has_value())
            {
                return createError("The 'header' field must be one of these strings: none, standard, or generic.");
            }

            if(arguments.contains("separator") && !arguments.at("separator").is_string())
            {
                return createError("The 'separator' field must be a one-character string.");
            }
            auto separator = ',';
            if(arguments.contains("separator"))
            {
                auto const separatorString = arguments.at("separator").get<std::string>();
                if(separatorString.size() != 1)
                {
                    return createError("The 'separator' field must be a one-character string.");
                }
                separator = separatorString.front();
            }

            if(arguments.contains("applyExtraThresholds") && !arguments.at("applyExtraThresholds").is_boolean())
            {
                return createError("The 'applyExtraThresholds' field must be a boolean.");
            }
            auto const applyExtraThresholds = arguments.value("applyExtraThresholds", true);

            if(arguments.contains("allowLargeOutput") && !arguments.at("allowLargeOutput").is_boolean())
            {
                return createError("The 'allowLargeOutput' field must be a boolean.");
            }
            auto const allowLargeOutput = arguments.value("allowLargeOutput", false);

            if(arguments.contains("waitSeconds") && !arguments.at("waitSeconds").is_number())
            {
                return createError("The 'waitSeconds' field must be a non-negative number.");
            }
            auto const waitSeconds = arguments.value("waitSeconds", 0);
            if(waitSeconds < 0)
            {
                return createError("The 'waitSeconds' field must be a non-negative number.");
            }

            constexpr size_t maxResultsWithoutLargeOutput = 20_z;

            auto const& identifiers = arguments.at("identifiers");
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            std::atomic<bool> shouldAbort{false};
            auto totalResultCountForRequest = 0_z;
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
                    nlohmann::json content;
                    content["type"] = "text";
                    content["trackId"] = identifier;
                    content["text"] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
                    response["content"].push_back(std::move(content));
                    continue;
                }

                auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                auto frameType = Track::Tools::getFrameType(trackAcsr);
                waitUntil(waitSeconds,
                          [&]()
                          {
                              auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                              auto const access = results.getReadAccess();
                              return static_cast<bool>(access) && (results.getMarkers() != nullptr || results.getPoints() != nullptr || results.getColumns() != nullptr);
                          });
                if(!frameType.has_value())
                {
                    response["isError"] = true;
                    nlohmann::json content;
                    content["type"] = "text";
                    content["trackId"] = identifier;
                    content["text"] = "The track type is unknown. The analysis might be running.";
                    response["content"].push_back(std::move(content));
                }
                else
                {
                    auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                    waitUntil(waitSeconds,
                              [&]()
                              {
                                  auto const access = results.getReadAccess();
                                  return static_cast<bool>(access);
                              });
                    auto const access = results.getReadAccess();
                    if(!static_cast<bool>(access))
                    {
                        response["isError"] = true;
                        nlohmann::json content;
                        content["type"] = "text";
                        content["trackId"] = identifier;
                        content["text"] = "The result data are being processed";
                        response["content"].push_back(std::move(content));
                        continue;
                    }

                    if(!allowLargeOutput)
                    {
                        if(results.isEmpty())
                        {
                            response["isError"] = true;
                            nlohmann::json content;
                            content["type"] = "text";
                            content["trackId"] = identifier;
                            content["text"] = "The result data are invalid";
                            response["content"].push_back(std::move(content));
                            continue;
                        }

                        auto const& extraThresholds = trackAcsr.getAttr<Track::AttrType::extraThresholds>();
                        auto const countResultsInChannels = [&](auto const& allResults)
                        {
                            using result_type = typename std::decay_t<decltype(allResults)>::element_type::value_type::value_type;
                            auto totalCount = 0_z;
                            for(auto channelIndex = 0_z; channelIndex < allResults->size(); ++channelIndex)
                            {
                                if(!channels.empty() && channels.count(channelIndex) == 0_z)
                                {
                                    continue;
                                }

                                auto const& channelResults = allResults->at(channelIndex);
                                if(timeRanges.empty())
                                {
                                    auto it = channelResults.cbegin();
                                    while(it != channelResults.cend())
                                    {
                                        if(!applyExtraThresholds || Track::Result::passThresholds(*it, extraThresholds))
                                        {
                                            ++totalCount;
                                        }
                                        ++it;
                                    }
                                }
                                else
                                {
                                    for(auto const& timeRange : timeRanges)
                                    {
                                        auto it = std::lower_bound(channelResults.cbegin(),
                                                                   channelResults.cend(),
                                                                   timeRange.getStart(),
                                                                   Track::Result::lower_cmp<result_type>);
                                        while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
                                        {
                                            if(!applyExtraThresholds || Track::Result::passThresholds(*it, extraThresholds))
                                            {
                                                ++totalCount;
                                            }
                                            ++it;
                                        }
                                    }
                                }
                            }
                            return totalCount;
                        };

                        auto totalResultCount = 0_z;
                        if(auto const markers = results.getMarkers())
                        {
                            totalResultCount = countResultsInChannels(markers);
                        }
                        else if(auto const points = results.getPoints())
                        {
                            totalResultCount = countResultsInChannels(points);
                        }
                        else if(auto const columns = results.getColumns())
                        {
                            totalResultCount = countResultsInChannels(columns);
                        }

                        totalResultCountForRequest += totalResultCount;
                        if(totalResultCountForRequest > maxResultsWithoutLargeOutput)
                        {
                            return createError("The request would export more than 20 results. Use 'get_track_results_summary' first, or set 'allowLargeOutput' to true only when strictly necessary.");
                        }
                    }

                    std::ostringstream stream;
                    stream << std::fixed;
                    stream << std::setprecision(4);
                    auto const exportResult = [&]() -> juce::Result
                    {
                        if(timeRanges.empty())
                        {
                            return Track::Exporter::toCsv(trackAcsr,
                                                          juce::Range<double>(-1.0, std::numeric_limits<double>::max()),
                                                          channels,
                                                          stream,
                                                          header.value(),
                                                          separator,
                                                          false,
                                                          applyExtraThresholds,
                                                          "\n",
                                                          false,
                                                          false,
                                                          shouldAbort);
                        }
                        for(auto const& timeRange : timeRanges)
                        {
                            stream << "Time Range: " << timeRange.getStart() << "s to " << timeRange.getEnd() << "s\n";
                            auto rresult = Track::Exporter::toCsv(trackAcsr,
                                                                  timeRange,
                                                                  channels,
                                                                  stream,
                                                                  header.value(),
                                                                  separator,
                                                                  false,
                                                                  applyExtraThresholds,
                                                                  "\n",
                                                                  false,
                                                                  false,
                                                                  shouldAbort);
                            stream << "\n";
                            if(rresult.failed())
                            {
                                return rresult;
                            }
                        }
                        return juce::Result::ok();
                    }();

                    nlohmann::json content;
                    content["type"] = "text";
                    content["trackId"] = identifier;
                    if(exportResult.failed())
                    {
                        response["isError"] = true;
                        content["text"] = exportResult.getErrorMessage();
                    }
                    else
                    {
                        content["text"] = stream.str();
                    }
                    response["content"].push_back(std::move(content));
                }
            }
            return response;
        }
        if(toolName == "save_track_results")
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
            if(arguments.at("identifiers").empty())
            {
                return createError("The 'identifiers' argument must not be empty.");
            }

            if(!arguments.contains("format") || !arguments.at("format").is_string())
            {
                return createError("The 'format' argument is required and must be one of these strings: jpeg, png, csv, lab, json, cue, reaper, puredata, max, sdif.");
            }

            Document::Exporter::Options options;
            auto const format = magic_enum::enum_cast<Document::Exporter::Options::Format>(arguments.at("format").get<std::string>());
            if(!format.has_value())
            {
                return createError("The 'format' argument must be one of these strings: jpeg, png, csv, lab, json, cue, reaper, puredata, max, sdif.");
            }
            options.format = *format;

            if(arguments.contains("useAutoSize"))
            {
                if(!arguments.at("useAutoSize").is_boolean())
                {
                    return createError("The 'useAutoSize' field must be a boolean.");
                }
                options.useAutoSize = arguments.at("useAutoSize").get<bool>();
            }

            if(arguments.contains("imageWidth"))
            {
                if(!arguments.at("imageWidth").is_number())
                {
                    return createError("The 'imageWidth' field must be a number.");
                }
                options.imageWidth = static_cast<int>(arguments.at("imageWidth").get<double>());
            }
            if(arguments.contains("imageHeight"))
            {
                if(!arguments.at("imageHeight").is_number())
                {
                    return createError("The 'imageHeight' field must be a number.");
                }
                options.imageHeight = static_cast<int>(arguments.at("imageHeight").get<double>());
            }
            if(arguments.contains("imagePpi"))
            {
                if(!arguments.at("imagePpi").is_number())
                {
                    return createError("The 'imagePpi' field must be a number.");
                }
                options.imagePpi = static_cast<int>(arguments.at("imagePpi").get<double>());
            }

            if(arguments.contains("csvHeaderType"))
            {
                if(!arguments.at("csvHeaderType").is_string())
                {
                    return createError("The 'csvHeaderType' field must be one of these strings: none, generic, specific.");
                }
                auto const csvHeaderType = magic_enum::enum_cast<Track::Exporter::CsvHeaderType>(arguments.at("csvHeaderType").get<std::string>());
                if(!csvHeaderType.has_value())
                {
                    return createError("The 'csvHeaderType' field must be one of these strings: none, generic, specific.");
                }
                options.csvHeaderType = *csvHeaderType;
            }

            if(arguments.contains("applyExtraThresholds"))
            {
                if(!arguments.at("applyExtraThresholds").is_boolean())
                {
                    return createError("The 'applyExtraThresholds' field must be a boolean.");
                }
                options.applyExtraThresholds = arguments.at("applyExtraThresholds").get<bool>();
            }

            if(arguments.contains("reaperType"))
            {
                if(!arguments.at("reaperType").is_string())
                {
                    return createError("The 'reaperType' field must be one of these strings: marker, region.");
                }
                auto const reaperType = magic_enum::enum_cast<Document::Exporter::Options::ReaperType>(arguments.at("reaperType").get<std::string>());
                if(!reaperType.has_value())
                {
                    return createError("The 'reaperType' field must be one of these strings: marker, region.");
                }
                options.reaperType = *reaperType;
            }

            if(arguments.contains("columnSeparator"))
            {
                if(!arguments.at("columnSeparator").is_string())
                {
                    return createError("The 'columnSeparator' field must be one of these strings: comma, space, tab, pipe, slash, colon.");
                }
                auto const columnSeparator = magic_enum::enum_cast<Document::Exporter::Options::ColumnSeparator>(arguments.at("columnSeparator").get<std::string>());
                if(!columnSeparator.has_value())
                {
                    return createError("The 'columnSeparator' field must be one of these strings: comma, space, tab, pipe, slash, colon.");
                }
                options.columnSeparator = *columnSeparator;
            }

            if(arguments.contains("labSeparator"))
            {
                if(!arguments.at("labSeparator").is_string())
                {
                    return createError("The 'labSeparator' field must be one of these strings: comma, space, tab, pipe, slash, colon.");
                }
                auto const labSeparator = magic_enum::enum_cast<Document::Exporter::Options::ColumnSeparator>(arguments.at("labSeparator").get<std::string>());
                if(!labSeparator.has_value())
                {
                    return createError("The 'labSeparator' field must be one of these strings: comma, space, tab, pipe, slash, colon.");
                }
                options.labSeparator = *labSeparator;
            }

            if(arguments.contains("disableLabelEscaping"))
            {
                if(!arguments.at("disableLabelEscaping").is_boolean())
                {
                    return createError("The 'disableLabelEscaping' field must be a boolean.");
                }
                options.disableLabelEscaping = arguments.at("disableLabelEscaping").get<bool>();
            }

            if(arguments.contains("includeDescription"))
            {
                if(!arguments.at("includeDescription").is_boolean())
                {
                    return createError("The 'includeDescription' field must be a boolean.");
                }
                options.includeDescription = arguments.at("includeDescription").get<bool>();
            }

            if(arguments.contains("sdifFrameSignature"))
            {
                if(!arguments.at("sdifFrameSignature").is_string())
                {
                    return createError("The 'sdifFrameSignature' field must be a string.");
                }
                options.sdifFrameSignature = arguments.at("sdifFrameSignature").get<std::string>();
            }

            if(arguments.contains("sdifMatrixSignature"))
            {
                if(!arguments.at("sdifMatrixSignature").is_string())
                {
                    return createError("The 'sdifMatrixSignature' field must be a string.");
                }
                options.sdifMatrixSignature = arguments.at("sdifMatrixSignature").get<std::string>();
            }

            if(arguments.contains("sdifColumnName"))
            {
                if(!arguments.at("sdifColumnName").is_string())
                {
                    return createError("The 'sdifColumnName' field must be a string.");
                }
                options.sdifColumnName = arguments.at("sdifColumnName").get<std::string>();
            }

            if(arguments.contains("outsideGridJustification"))
            {
                if(!arguments.at("outsideGridJustification").is_array())
                {
                    return createError("The 'outsideGridJustification' field must be an array containing any of these strings: none, left, right, top, bottom.");
                }
                auto justification = Zoom::Grid::Justification(Zoom::Grid::Justification::none);
                auto const& justificationValues = arguments.at("outsideGridJustification");
                if(!justificationValues.empty())
                {
                    for(auto const& justificationValue : justificationValues)
                    {
                        if(!justificationValue.is_string())
                        {
                            return createError("The 'outsideGridJustification' field must be an array containing any of these strings: none, left, right, top, bottom.");
                        }

                        auto const flagName = justificationValue.get<std::string>();
                        if(flagName == "none")
                        {
                            if(justificationValues.size() > 1_z)
                            {
                                return createError("The 'outsideGridJustification' field cannot mix 'none' with other flags.");
                            }
                            justification = Zoom::Grid::Justification(Zoom::Grid::Justification::none);
                            break;
                        }

                        auto flag = Zoom::Grid::Justification::none;
                        if(flagName == "left")
                        {
                            flag = Zoom::Grid::Justification::left;
                        }
                        else if(flagName == "right")
                        {
                            flag = Zoom::Grid::Justification::right;
                        }
                        else if(flagName == "top")
                        {
                            flag = Zoom::Grid::Justification::top;
                        }
                        else if(flagName == "bottom")
                        {
                            flag = Zoom::Grid::Justification::bottom;
                        }
                        else
                        {
                            return createError("The 'outsideGridJustification' field must be an array containing any of these strings: none, left, right, top, bottom.");
                        }
                        justification = Zoom::Grid::Justification(justification.getFlags() | flag);
                    }
                }
                options.outsideGridJustification = justification;
            }

            if(!options.isValid())
            {
                return createError("The 'sdifFrameSignature' and 'sdifMatrixSignature' fields must be 4-character signatures without '?'.");
            }

            std::set<juce::String> identifiers;
            auto const& identifierValues = arguments.at("identifiers");
            for(auto const& identifierValue : identifierValues)
            {
                if(!identifierValue.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }
                identifiers.insert(identifierValue.get<std::string>());
            }
            if(identifiers.empty())
            {
                return createError("The 'identifiers' argument must not be empty.");
            }

            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            for(auto const& identifier : identifiers)
            {
                if(!Document::Tools::hasItem(documentAcsr, identifier))
                {
                    return createError(juce::String(R"(The item (group or track) "ITEMID" doesn't exist.)").replace("ITEMID", identifier));
                }
            }

            auto const tempDirectory = getTempExportDirectory().getChildFile(juce::Uuid().toString());
            auto const directoryResult = tempDirectory.createDirectory();
            if(directoryResult.failed())
            {
                return createError(directoryResult.getErrorMessage());
            }

            std::atomic<bool> shouldAbort{false};
            auto const exportResult = Document::Exporter::exportTo(documentAcsr, tempDirectory, {}, {}, "", identifiers, options, shouldAbort);
            if(exportResult.failed())
            {
                return createError(exportResult.getErrorMessage());
            }

            nlohmann::json payload;
            payload["directory"] = tempDirectory.getFullPathName().toStdString();
            payload["files"] = nlohmann::json::array();
            for(auto const& file : tempDirectory.findChildFiles(juce::File::findFiles, false, "*"))
            {
                nlohmann::json item;
                item["file"] = file.getFullPathName().toStdString();
                payload["files"].push_back(std::move(item));
            }

            if(payload["files"].empty())
            {
                return createError("The export completed successfully, but no files were created.");
            }

            nlohmann::json content;
            content["type"] = "text";
            content["text"] = payload.dump();
            response["content"].push_back(std::move(content));
            return response;
        }
        if(toolName == "set_track_results_raw")
        {
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");

            if(arguments.contains("separator") && (!arguments.at("separator").is_string() || arguments.at("separator").get<std::string>().size() != 1_z))
            {
                return createError("The 'separator' field must be a one-character string.");
            }
            auto const separator = arguments.contains("separator") ? arguments.at("separator").get<std::string>().front() : ',';

            if(arguments.contains("timeRange") && (!arguments.at("timeRange").is_array() || arguments.at("timeRange").size() != 2_z || !arguments.at("timeRange").at(0_z).is_number() || !arguments.at("timeRange").at(1_z).is_number()))
            {
                return createError("The 'timeRange' field must be an array of 2 numbers for startTime and endTime.");
            }
            auto const startTime = arguments.contains("timeRange") ? arguments.at("timeRange").at(0_z).get<double>() : 0.0;
            auto const endTime = arguments.contains("timeRange") ? arguments.at("timeRange").at(1_z).get<double>() : std::numeric_limits<double>::max();
            if(startTime > endTime)
            {
                return createError("Each 'timeRanges' startTime must be less than or equal to endTime.");
            }
            juce::Range<double> const timeRange{startTime, endTime};

            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            ScopedDocumentAction actionGuard(documentDir);
            if(!arguments.contains("identifier") || !arguments.at("identifier").is_string())
            {
                return createError("The 'identifier' field is required and must be a string.");
            }
            if(!arguments.contains("results") || !arguments.at("results").is_string())
            {
                return createError("The `results` field is required and must be an CSV string.");
            }

            auto const fillResponse = [&](bool isError, juce::String message)
            {
                if(isError)
                {
                    actionGuard.abort();
                    response["isError"] = true;
                }

                nlohmann::json content;
                content["type"] = "text";
                nlohmann::json payload;
                payload["message"] = message;
                content["text"] = payload.dump();
                response["content"].push_back(std::move(content));
            };

            auto const identifier = arguments.at("identifier").get<std::string>();
            if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
            {
                fillResponse(true, juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                return response;
            }

            auto const results = arguments.at("results").get<std::string>();
            auto stream = std::istringstream(results);
            if(!stream || !stream.good())
            {
                fillResponse(true, juce::String(R"(Failed to parse CSV for track "TRACKID": REASON)").replace("TRACKID", identifier).replace("REASON", "The input stream of cannot be opened."));
                return response;
            }

            std::atomic<bool> shouldAbort{false};
            std::atomic<float> advancement{0.0f};
            auto loadedResults = Track::Loader::loadFromCsv(stream, separator, false, '\n', false, shouldAbort, advancement);
            if(auto* error = std::get_if<juce::String>(&loadedResults))
            {
                fillResponse(true, juce::String(R"(Failed to parse CSV for track "TRACKID": REASON)").replace("TRACKID", identifier).replace("REASON", *error));
                return response;
            }

            auto const& resultData = std::get<0>(loadedResults);
            auto const access = resultData.getReadAccess();
            if(!static_cast<bool>(access))
            {
                fillResponse(true, juce::String(R"(Failed to parse CSV for track "TRACKID": REASON)").replace("TRACKID", identifier).replace("REASON", "The data cannot be accessed."));
                return response;
            }

            auto& undoManager = Instance::get().getUndoManager();
            auto safeTrackAccessorFn = documentDir.getSafeTrackAccessorFn(identifier);
            undoManager.perform(std::make_unique<Track::Result::Modifier::ActionErase>(safeTrackAccessorFn, 0_z, timeRange, false, timeRange.getEnd()).release());
            auto applyAllChannels = [&](auto const allData)
            {
                for(auto index = 0_z; index < allData->size(); ++index)
                {
                    auto const& channelData = allData->at(index);
                    undoManager.perform(std::make_unique<Track::Result::Modifier::ActionPaste>(safeTrackAccessorFn, index, channelData, timeRange.getStart(), false, timeRange.getEnd()).release());
                }
            };
            if(auto const allMarkers = resultData.getMarkers())
            {
                applyAllChannels(allMarkers);
            }
            else if(auto const allPoints = resultData.getPoints())
            {
                applyAllChannels(allPoints);
            }
            else if(auto const allColumns = resultData.getColumns())
            {
                applyAllChannels(allColumns);
            }
            else
            {
                fillResponse(true, juce::String(R"(Failed to parse CSV for track "TRACKID": REASON)").replace("TRACKID", identifier).replace("REASON", "The data is empty."));
                return response;
            }

            actionGuard.commit(juce::translate("Set track results (Neuralyzer)"));
            fillResponse(false, juce::String(R"(Updated raw results for track "TRACKID".)").replace("TRACKID", identifier));
            return response;
        }
        if(toolName == "get_track_results_summary")
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

            auto timeRangesResults = getTimeRangesFromArguments(arguments);
            if(timeRangesResults.index() == 1)
            {
                return std::get<1>(timeRangesResults);
            }
            auto const timeRanges = std::get<0>(timeRangesResults);

            auto channelResult = getChannelsFromArguments(arguments);
            if(channelResult.index() == 1)
            {
                return std::get<1>(channelResult);
            }
            auto const channels = std::get<0>(channelResult);

            if(arguments.contains("applyExtraThresholds") && !arguments.at("applyExtraThresholds").is_boolean())
            {
                return createError("The 'applyExtraThresholds' field must be a boolean.");
            }
            auto const applyExtraThresholds = arguments.value("applyExtraThresholds", true);

            if(arguments.contains("waitSeconds") && !arguments.at("waitSeconds").is_number())
            {
                return createError("The 'waitSeconds' field must be a non-negative number.");
            }
            auto const waitSeconds = arguments.value("waitSeconds", 0);
            if(waitSeconds < 0)
            {
                return createError("The 'waitSeconds' field must be a non-negative number.");
            }

            auto const& identifiers = arguments.at("identifiers");
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            nlohmann::json stats = nlohmann::json::array();
            for(auto const& identifierJson : identifiers)
            {
                if(!identifierJson.is_string())
                {
                    return createError("The 'identifiers' argument is required and must be an array of strings.");
                }

                auto const identifier = identifierJson.get<std::string>();
                nlohmann::json trackStats;
                trackStats["trackId"] = identifier;
                if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    response["isError"] = true;
                    trackStats["error"] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
                    stats.push_back(std::move(trackStats));
                    continue;
                }

                auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                auto frameType = Track::Tools::getFrameType(trackAcsr);
                waitUntil(waitSeconds,
                          [&]()
                          {
                              auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                              auto const access = results.getReadAccess();
                              return static_cast<bool>(access) && (results.getMarkers() != nullptr || results.getPoints() != nullptr || results.getColumns() != nullptr);
                          });
                if(!frameType.has_value())
                {
                    response["isError"] = true;
                    trackStats["error"] = "The track type is unknown. The analysis might be running.";
                    stats.push_back(std::move(trackStats));
                    continue;
                }

                auto const& results = trackAcsr.getAttr<Track::AttrType::results>();
                waitUntil(waitSeconds,
                          [&]()
                          {
                              auto const access = results.getReadAccess();
                              return static_cast<bool>(access);
                          });
                auto const access = results.getReadAccess();
                if(!static_cast<bool>(access))
                {
                    response["isError"] = true;
                    trackStats["error"] = "The result data are being processed";
                    stats.push_back(std::move(trackStats));
                    continue;
                }
                if(results.isEmpty())
                {
                    response["isError"] = true;
                    trackStats["error"] = "The result data are invalid";
                    stats.push_back(std::move(trackStats));
                    continue;
                }

                auto const& extraThresholds = trackAcsr.getAttr<Track::AttrType::extraThresholds>();
                auto const& extraOutputs = trackAcsr.getAttr<Track::AttrType::description>().extraOutputs;

                auto const iterateResults = [&](std::string const& name, auto const& allResults, std::function<void(void)> onRangeBegin, std::function<void(typename std::decay_t<decltype(allResults)>::element_type::value_type::value_type const&)> onResultAdvance, std::function<nlohmann::json(void)> onRangeEnd)
                {
                    trackStats["type"] = name;
                    trackStats["channels"] = nlohmann::json::array();
                    using result_type = typename std::decay_t<decltype(allResults)>::element_type::value_type::value_type;

                    auto const performRange = [&](nlohmann::json& payload, auto const& channelResults, auto const& timeRange)
                    {
                        payload["value"] = nlohmann::json::object();
                        payload["extraValues"] = nlohmann::json::array();
                        if(onRangeBegin != nullptr)
                        {
                            onRangeBegin();
                        }

                        auto resultCount = 0_z;
                        std::vector<std::vector<float>> extraValues(extraOutputs.size());
                        auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Track::Result::lower_cmp<result_type>);
                        // Iterate over results
                        while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
                        {
                            // Check if the result is accepted
                            if(!applyExtraThresholds || Track::Result::passThresholds(*it, extraThresholds))
                            {
                                if(std::get<3_z>(*it).size() > extraValues.size())
                                {
                                    extraValues.resize(std::get<3_z>(*it).size());
                                }
                                for(auto index = 0_z; index < std::get<3_z>(*it).size(); ++index)
                                {
                                    extraValues.at(index).push_back(std::get<3_z>(*it).at(index));
                                }
                                if(onResultAdvance != nullptr)
                                {
                                    onResultAdvance(*it);
                                }
                                ++resultCount;
                            }
                            ++it;
                        }
                        payload["count"] = resultCount;
                        for(auto index = 0_z; index < extraValues.size(); ++index)
                        {
                            nlohmann::json extraValueStats;
                            extraValueStats["index"] = index;
                            if(index < extraOutputs.size())
                            {
                                auto const& extraOutput = extraOutputs.at(index);
                                extraValueStats["identifier"] = extraOutput.identifier;
                                extraValueStats["name"] = extraOutput.name.empty() ? juce::String("Extra INDEX").replace("INDEX", juce::String(index + 1_z)) : juce::String(extraOutput.name);
                                extraValueStats["unit"] = extraOutput.unit;
                            }
                            else
                            {
                                extraValueStats["identifier"] = "";
                                extraValueStats["name"] = juce::String("Extra INDEX").replace("INDEX", juce::String(index + 1_z));
                                extraValueStats["unit"] = "";
                            }
                            extraValueStats["stats"] = statisticsPayload(extraValues.at(index));
                            payload["extraValues"].push_back(std::move(extraValueStats));
                        }

                        if(onRangeEnd != nullptr)
                        {
                            payload["value"] = onRangeEnd();
                        }
                    };

                    // Iterate over channels
                    for(auto channelIndex = 0_z; channelIndex < allResults->size(); ++channelIndex)
                    {
                        // Check if the channel is accepted
                        if(channels.empty() || channels.count(channelIndex) > 0_z)
                        {
                            auto const& channelResults = allResults->at(channelIndex);
                            nlohmann::json channelPayload;
                            channelPayload["channel"] = channelIndex;
                            if(timeRanges.empty())
                            {
                                performRange(channelPayload, channelResults, juce::Range<double>(0.0, std::numeric_limits<double>::max()));
                            }
                            else
                            {
                                for(auto const& timeRange : timeRanges)
                                {
                                    nlohmann::json rangePayload;
                                    rangePayload["timeRange"].push_back(timeRange.getStart());
                                    rangePayload["timeRange"].push_back(timeRange.getEnd());
                                    performRange(rangePayload, channelResults, timeRange);
                                    channelPayload["timeRanges"].push_back(std::move(rangePayload));
                                }
                            }
                            trackStats["channels"].push_back(std::move(channelPayload));
                        }
                    }
                };

                if(auto const markers = results.getMarkers())
                {
                    iterateResults("label", markers, nullptr, nullptr, nullptr);
                }
                else if(auto const points = results.getPoints())
                {
                    std::vector<float> values;
                    iterateResults("value", points, [&]()
                                   {
                                       values.clear();
                                   },
                                   [&](Track::Results::Point const& point)
                                   {
                                       if(std::get<2_z>(point).has_value())
                                       {
                                           values.push_back(std::get<2_z>(point).value());
                                       }
                                   },
                                   [&]()
                                   {
                                       return statisticsPayload(values);
                                   });
                }
                else if(auto const columns = results.getColumns())
                {
                    std::vector<float> values;
                    iterateResults("vector", columns, [&]()
                                   {
                                       values.clear();
                                   },
                                   [&](Track::Results::Column const& column)
                                   {
                                       values.insert(values.end(), std::get<2_z>(column).begin(), std::get<2_z>(column).end());
                                   },
                                   [&]()
                                   {
                                       return statisticsPayload(values);
                                   });
                }
                stats.push_back(std::move(trackStats));
            }

            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["stats"] = std::move(stats);
            content["text"] = payload.dump();
            response["content"].push_back(std::move(content));
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
                    if(Track::Tools::supportsInputTracks(trackAcsr))
                    {
                        inputTracks[identifier] = trackAcsr.getAttr<Track::AttrType::inputs>();
                    }
                    else
                    {
                        inputTracks[identifier] = juce::String(R"(The track "TRACKID" doesn't support input track.)").replace("TRACKID", identifier);
                    }
                }
                else
                {
                    response["isError"] = true;
                    inputTracks[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
                    visibilityByTrack[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
            if(!arguments.contains("tracks") || !arguments.at("tracks").is_array())
            {
                return createError("The 'tracks' argument is required and must be an array of object.");
            }
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const& hierarchyManager = documentDir.getHierarchyManager();
            nlohmann::json results;
            for(auto const& track : arguments.at("tracks"))
            {
                if(!track.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!track.contains("receiver_track_identifier") || !track.at("receiver_track_identifier").is_string())
                {
                    return createError("The 'receiver_track_identifier' field is required and must be a string.");
                }
                if(!track.contains("receiver_input_identifier") || !track.at("receiver_input_identifier").is_string())
                {
                    return createError("The 'receiver_input_identifier' field is required and must be a string.");
                }
                auto const receiverTrackIdentifier = track.at("receiver_track_identifier").get<std::string>();
                auto const receiverInputIdentifier = track.at("receiver_input_identifier").get<std::string>();
                auto const compatibleTracks = hierarchyManager.getAvailableTracksFor(receiverTrackIdentifier, receiverInputIdentifier);
                nlohmann::json trackArray = nlohmann::json::array();
                for(auto const& trackInfo : compatibleTracks)
                {
                    trackArray.push_back(trackInfo.identifier);
                }
                results[receiverTrackIdentifier][receiverInputIdentifier] = trackArray;
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.dump();
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
                    thresholdsByTrack[identifier] = juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier);
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
            ScopedDocumentAction actionGuard(documentDir);
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
                auto resolvedTrackJson = trackJson;
                for(auto const& fieldName : {"colorMap", "colorBackground", "colorForeground", "colorDuration", "colorText", "colorShadow", "fontName", "fontSize", "fontStyle", "lineWidth", "unit", "labelLayoutPosition", "labelLayoutJustification"})
                {
                    if(!resolvedTrackJson.contains(fieldName) && arguments.contains(fieldName))
                    {
                        resolvedTrackJson[fieldName] = arguments.at(fieldName);
                    }
                }
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    auto const frameType = Track::Tools::getFrameType(trackAcsr);
                    if(!frameType.has_value())
                    {
                        response["isError"] = true;
                        results.add(juce::String(R"(The track "TRACKID" doesn't support the "colorMap" property.)").replace("TRACKID", identifier));
                        break;
                    }
                    auto settings = trackAcsr.getAttr<Track::AttrType::graphicsSettings>();
                    auto const isLabel = frameType.has_value() || frameType.value() == Track::FrameType::label;
                    auto const isValue = frameType.has_value() || frameType.value() == Track::FrameType::value;
                    auto const isVector = frameType.has_value() || frameType.value() == Track::FrameType::vector;

                    // Colour map (vector tracks only)
                    if(resolvedTrackJson.contains("colorMap"))
                    {
                        if(!resolvedTrackJson.at("colorMap").is_string())
                        {
                            return createError("The 'colorMap' field must be a string.");
                        }
                        auto const colorMapStr = resolvedTrackJson.at("colorMap").get<std::string>();
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
                            results.add(juce::String(R"(The track "TRACKID" doesn't support the "colorMap" property.)").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    auto const applyColour = [&](const char* fieldName, auto& refValue, bool isCompatible)
                    {
                        auto const colourStr = resolvedTrackJson.at(fieldName).get<std::string>();
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
                        results.add(juce::String(R"(The track "TRACKID" doesn't support the "FIELDNAME" property.)").replace("TRACKID", identifier).replace("FIELDNAME", fieldName));
                        return false;
                    };

                    // Colours (label and value tracks)
                    if(resolvedTrackJson.contains("colorBackground"))
                    {
                        if(!resolvedTrackJson.at("colorBackground").is_string())
                        {
                            return createError("The 'colorBackground' field must be a string.");
                        }
                        if(!applyColour("colorBackground", settings.colours.background, isLabel || isValue))
                        {
                            break;
                        }
                    }
                    if(resolvedTrackJson.contains("colorForeground"))
                    {
                        if(!resolvedTrackJson.at("colorForeground").is_string())
                        {
                            return createError("The 'colorForeground' field must be a string.");
                        }
                        if(!applyColour("colorForeground", settings.colours.foreground, isLabel || isValue))
                        {
                            break;
                        }
                    }
                    if(resolvedTrackJson.contains("colorDuration"))
                    {
                        if(!resolvedTrackJson.at("colorDuration").is_string())
                        {
                            return createError("The 'colorDuration' field must be a string.");
                        }
                        if(!applyColour("colorDuration", settings.colours.duration, isLabel))
                        {
                            break;
                        }
                    }
                    if(resolvedTrackJson.contains("colorText"))
                    {
                        if(!resolvedTrackJson.at("colorText").is_string())
                        {
                            return createError("The 'colorText' field must be a string.");
                        }
                        if(!applyColour("colorText", settings.colours.text, isLabel))
                        {
                            break;
                        }
                    }
                    if(resolvedTrackJson.contains("colorShadow"))
                    {
                        if(!resolvedTrackJson.at("colorShadow").is_string())
                        {
                            return createError("The 'colorShadow' field must be a string.");
                        }
                        if(!applyColour("colorShadow", settings.colours.shadow, isLabel))
                        {
                            break;
                        }
                    }

                    // Font (label and value tracks)
                    if(resolvedTrackJson.contains("fontName") || resolvedTrackJson.contains("fontSize") || resolvedTrackJson.contains("fontStyle"))
                    {
                        auto fontName = settings.font.getName();
                        auto fontStyle = settings.font.getStyle();
                        auto fontSize = settings.font.getHeight();

                        if(resolvedTrackJson.contains("fontName"))
                        {
                            if(!resolvedTrackJson.at("fontName").is_string())
                            {
                                return createError("The 'fontName' field must be a string.");
                            }
                            fontName = juce::String(resolvedTrackJson.at("fontName").get<std::string>());
                        }
                        if(resolvedTrackJson.contains("fontStyle"))
                        {
                            if(!resolvedTrackJson.at("fontStyle").is_string())
                            {
                                return createError("The 'fontStyle' field must be a string.");
                            }
                            fontStyle = juce::String(resolvedTrackJson.at("fontStyle").get<std::string>());
                        }
                        if(resolvedTrackJson.contains("fontSize"))
                        {
                            if(!resolvedTrackJson.at("fontSize").is_number())
                            {
                                return createError("The 'fontSize' field must be a floating point number.");
                            }
                            fontSize = resolvedTrackJson.at("fontSize").get<float>();
                        }

                        if(isLabel || isValue)
                        {
                            settings.font = juce::FontOptions(fontName, fontStyle, fontSize);
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String(R"(The track "TRACKID" doesn't support the fonts properties.)").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Line width (label and value tracks)
                    if(resolvedTrackJson.contains("lineWidth"))
                    {
                        if(!resolvedTrackJson.at("lineWidth").is_number())
                        {
                            return createError("The 'lineWidth' field must be a floating point number.");
                        }
                        if(isLabel || isValue)
                        {
                            settings.lineWidth = resolvedTrackJson.at("lineWidth").get<float>();
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String(R"(The track "TRACKID" doesn't support the lineWidth property.)").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Unit (value and vector tracks)
                    if(resolvedTrackJson.contains("unit"))
                    {
                        if(!resolvedTrackJson.at("unit").is_string())
                        {
                            return createError("The 'unit' field must be a string.");
                        }
                        if(isValue || isValue)
                        {
                            auto const unitStr = juce::String(resolvedTrackJson.at("unit").get<std::string>());
                            settings.unit = unitStr.isEmpty() ? std::nullopt : std::optional<juce::String>(unitStr);
                        }
                        else
                        {
                            response["isError"] = true;
                            results.add(juce::String(R"(The track "TRACKID" doesn't support the unit property.)").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    // Label layout (label tracks only)
                    if(resolvedTrackJson.contains("labelLayoutPosition") || resolvedTrackJson.contains("labelLayoutJustification"))
                    {
                        auto position = settings.labelLayout.position;
                        auto justification = settings.labelLayout.justification;
                        if(resolvedTrackJson.contains("labelLayoutPosition"))
                        {
                            if(!resolvedTrackJson.at("labelLayoutPosition").is_number())
                            {
                                return createError("The 'labelLayoutPosition' field must be a floating point number.");
                            }
                            position = resolvedTrackJson.at("labelLayoutPosition").get<float>();
                        }
                        if(resolvedTrackJson.contains("labelLayoutJustification"))
                        {
                            if(!resolvedTrackJson.at("labelLayoutJustification").is_string())
                            {
                                return createError("The 'labelLayoutJustification' field must be a string.");
                            }
                            auto const justificationStr = resolvedTrackJson.at("labelLayoutJustification").get<std::string>();
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
                            results.add(juce::String(R"(The track "TRACKID" doesn't support the label layout properties.)").replace("TRACKID", identifier));
                            break;
                        }
                    }

                    trackAcsr.setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
                    results.add(juce::String(R"(The graphics settings of track "TRACKID" have been updated.)").replace("TRACKID", identifier));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Change track graphics (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
            }
            nlohmann::json content;
            content["type"] = "text";
            content["text"] = results.joinIntoString("\n");
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
            ScopedDocumentAction actionGuard(documentDir);
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
                    results.add(juce::String(R"(The track "TRACKID" has been renamed "NAME".)").replace("TRACKID", identifier).replace("NAME", name));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Change track name (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            if(arguments.contains("parameters") && !arguments.at("parameters").is_array())
            {
                return createError("The 'parameters' field must be an array of objects.");
            }
            auto const& tracks = arguments.at("tracks");
            auto const& defaultParameters = arguments.contains("parameters") ? arguments.at("parameters") : nlohmann::json::array();
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            ScopedDocumentAction actionGuard(documentDir);
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
                if(trackJson.contains("parameters") && !trackJson.at("parameters").is_array())
                {
                    return createError("The 'parameters' field must be an array of objects.");
                }
                auto const& trackParameters = trackJson.contains("parameters") ? trackJson.at("parameters") : nlohmann::json::array();
                if(trackParameters.empty() && defaultParameters.empty())
                {
                    return createError("The 'parameters' field is required either at the top-level or per track.");
                }
                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                    auto const& description = trackAcsr.getAttr<Track::AttrType::description>();
                    auto state = trackAcsr.getAttr<Track::AttrType::state>();
                    auto hasTrackError = false;

                    // Apply global defaults first; unsupported parameters are ignored for this track.
                    for(auto const& parameterJson : defaultParameters)
                    {
                        if(!parameterJson.is_object())
                        {
                            return createError("The 'parameters' field must contain an array of objects.");
                        }
                        if(!parameterJson.contains("key") || !parameterJson.at("key").is_string())
                        {
                            return createError("The 'key' field is required and must be a string.");
                        }
                        if(!parameterJson.contains("value") || !parameterJson.at("value").is_number())
                        {
                            return createError("The 'value' field is required and must be a number.");
                        }

                        auto const parameter = parameterJson.at("key").get<std::string>();
                        auto const value = parameterJson.at("value").get<float>();
                        auto const paramIt = std::find_if(description.parameters.cbegin(), description.parameters.cend(),
                                                          [&parameter](auto const& param)
                                                          {
                                                              return param.identifier == parameter;
                                                          });
                        if(paramIt == description.parameters.cend())
                        {
                            continue;
                        }

                        auto const clampedValue = std::clamp(value, paramIt->minValue, paramIt->maxValue);
                        state.parameters[parameter] = clampedValue;
                    }

                    // Apply per-track parameters on top of globals; unknown keys are errors.
                    for(auto const& parameterJson : trackParameters)
                    {
                        if(!parameterJson.is_object())
                        {
                            return createError("The 'parameters' field must contain an array of objects.");
                        }
                        if(!parameterJson.contains("key") || !parameterJson.at("key").is_string())
                        {
                            return createError("The 'key' field is required and must be a string.");
                        }
                        if(!parameterJson.contains("value") || !parameterJson.at("value").is_number())
                        {
                            return createError("The 'value' field is required and must be a number.");
                        }

                        auto const parameter = parameterJson.at("key").get<std::string>();
                        auto const value = parameterJson.at("value").get<float>();
                        auto const paramIt = std::find_if(description.parameters.cbegin(), description.parameters.cend(),
                                                          [&parameter](auto const& param)
                                                          {
                                                              return param.identifier == parameter;
                                                          });
                        if(paramIt == description.parameters.cend())
                        {
                            response["isError"] = true;
                            results.add(juce::String(R"(The parameter "PARAM" of the track "TRACKID" doesn't exist.)").replace("TRACKID", identifier).replace("PARAM", parameter));
                            hasTrackError = true;
                            break;
                        }

                        auto const clampedValue = std::clamp(value, paramIt->minValue, paramIt->maxValue);
                        state.parameters[parameter] = clampedValue;
                        results.add(juce::String(R"(The parameter "PARAM" of the track "TRACKID" has been set to "VAL".)").replace("TRACKID", identifier).replace("PARAM", parameter).replace("VAL", juce::String(clampedValue)));
                    }

                    if(hasTrackError)
                    {
                        break;
                    }

                    trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Change track parameters (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            if(arguments.contains("receiver_input_identifier") && !arguments.at("receiver_input_identifier").is_string())
            {
                return createError("The 'receiver_input_identifier' field must be a string.");
            }
            if(arguments.contains("sender_track_identifier") && !arguments.at("sender_track_identifier").is_string())
            {
                return createError("The 'sender_track_identifier' field must be a string.");
            }
            auto const& tracks = arguments.at("tracks");
            auto const defaultReceiverInputIdentifier = juce::String(arguments.value("receiver_input_identifier", std::string{}));
            auto const defaultSenderTrackIdentifier = juce::String(arguments.value("sender_track_identifier", std::string{}));
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const& hierarchyManager = documentDir.getHierarchyManager();
            ScopedDocumentAction actionGuard(documentDir);
            juce::StringArray results;
            for(auto const& trackJson : tracks)
            {
                if(!trackJson.is_object())
                {
                    return createError("The 'tracks' argument is required and must be an array of objects.");
                }
                if(!trackJson.contains("receiver_track_identifier") || !trackJson.at("receiver_track_identifier").is_string())
                {
                    return createError("The 'receiver_track_identifier' field is required and must be a string.");
                }
                if(trackJson.contains("receiver_input_identifier") && !trackJson.at("receiver_input_identifier").is_string())
                {
                    return createError("The 'receiver_input_identifier' field must be a string.");
                }
                if(!trackJson.contains("receiver_input_identifier") && defaultReceiverInputIdentifier.isEmpty())
                {
                    return createError("The 'receiver_input_identifier' field is required either at the top-level or per track.");
                }
                if(trackJson.contains("sender_track_identifier") && !trackJson.at("sender_track_identifier").is_string())
                {
                    return createError("The 'sender_track_identifier' field must be a string.");
                }
                if(!trackJson.contains("sender_track_identifier") && defaultSenderTrackIdentifier.isEmpty())
                {
                    return createError("The 'sender_track_identifier' field is required either at the top-level or per track.");
                }
                auto const receiverTrackIdentifier = juce::String(trackJson.at("receiver_track_identifier").get<std::string>());
                auto const receiverInputIdentifier = trackJson.contains("receiver_input_identifier")
                                                         ? juce::String(trackJson.at("receiver_input_identifier").get<std::string>())
                                                         : defaultReceiverInputIdentifier;
                auto const senderTrackIdentifier = trackJson.contains("sender_track_identifier")
                                                       ? juce::String(trackJson.at("sender_track_identifier").get<std::string>())
                                                       : defaultSenderTrackIdentifier;
                if(!Document::Tools::hasTrackAcsr(documentAcsr, receiverTrackIdentifier))
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The receiver track "RTRACKID" doesn't exist.)").replace("RTRACKID", receiverTrackIdentifier));
                }
                else if(senderTrackIdentifier.isNotEmpty() && !Document::Tools::hasTrackAcsr(documentAcsr, senderTrackIdentifier))
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The sender track "STRACKID" doesn't exist.)").replace("STRACKID", senderTrackIdentifier));
                }
                else
                {
                    auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, receiverTrackIdentifier);
                    if(!Track::Tools::supportsInputTrack(trackAcsr, receiverInputIdentifier))
                    {
                        response["isError"] = true;
                        results.add(juce::String(R"(The receiver track "RTRACKID" doesn't support the input "INPUTID".)").replace("RTRACKID", receiverTrackIdentifier).replace("INPUTID", receiverInputIdentifier));
                        break;
                    }
                    if(senderTrackIdentifier.isNotEmpty() && !hierarchyManager.isTrackValidFor(receiverTrackIdentifier, receiverInputIdentifier, senderTrackIdentifier))
                    {
                        response["isError"] = true;
                        results.add(juce::String(R"(The receiver track "RTRACKID" doesn't support the sender track "STRACKID" for the input "INPUTID" (unsupported type or circular dependency).)").replace("RTRACKID", receiverTrackIdentifier).replace("STRACKID", senderTrackIdentifier).replace("INPUTID", receiverInputIdentifier));
                        break;
                    }
                    auto inputs = trackAcsr.getAttr<Track::AttrType::inputs>();
                    inputs[receiverInputIdentifier] = senderTrackIdentifier;
                    trackAcsr.setAttr<Track::AttrType::inputs>(inputs, NotificationType::synchronous);
                    if(senderTrackIdentifier.isEmpty())
                    {
                        results.add(juce::String(R"(The input "INPUTID" of the receiver track "RTRACKID" input has been disconnected.)").replace("INPUTID", receiverInputIdentifier).replace("RTRACKID", receiverInputIdentifier));
                    }
                    else
                    {
                        results.add(juce::String(R"(The receiver track "RTRACKID" now uses the sender track "STRACKID" for the input "INPUTID".)").replace("RTRACKID", receiverInputIdentifier).replace("STRACKID", senderTrackIdentifier).replace("INPUTID", receiverInputIdentifier));
                    }
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Set track input (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            ScopedDocumentAction actionGuard(documentDir);
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
                    results.add(juce::String(R"(The track "TRACKID" visibility in group has been set to "STATE".)").replace("TRACKID", identifier).replace("STATE", visibleInGroup ? "true" : "false"));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Set track visibility in group (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            if(arguments.contains("extraThresholds") && !arguments.at("extraThresholds").is_array())
            {
                return createError("The 'extraThresholds' field must be an array.");
            }
            auto const& tracks = arguments.at("tracks");
            auto const& defaultExtraThresholds = arguments.contains("extraThresholds") ? arguments.at("extraThresholds") : nlohmann::json::array();
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            ScopedDocumentAction actionGuard(documentDir);
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
                if(trackJson.contains("extraThresholds") && !trackJson.at("extraThresholds").is_array())
                {
                    return createError("The 'extraThresholds' field must be an array.");
                }
                if(!trackJson.contains("extraThresholds") && defaultExtraThresholds.empty())
                {
                    return createError("The 'extraThresholds' field is required either at the top-level or per track.");
                }

                auto const identifier = juce::String(trackJson.at("identifier").get<std::string>());
                auto const& thresholdsArray = trackJson.contains("extraThresholds") ? trackJson.at("extraThresholds") : defaultExtraThresholds;

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
                    results.add(juce::String(R"(The extra thresholds of track "TRACKID" have been updated.)").replace("TRACKID", identifier));
                }
                else
                {
                    response["isError"] = true;
                    results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Set track extra thresholds (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
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
            if(arguments.contains("plugin") && !arguments.at("plugin").is_object())
            {
                return createError("The 'plugin' field must be an object.");
            }
            if(arguments.contains("plugin"))
            {
                auto const& plugin = arguments.at("plugin");
                if(!plugin.contains("identifier") || !plugin.at("identifier").is_string() || !plugin.contains("feature") || !plugin.at("feature").is_string())
                {
                    return createError("The 'plugin' field must contain the 'identifier' and 'feature' string fields.");
                }
            }
            if(arguments.contains("groupIdentifier") && !arguments.at("groupIdentifier").is_string())
            {
                return createError("The 'groupIdentifier' field must be a string.");
            }
            if(arguments.contains("parameters") && !arguments.at("parameters").is_array())
            {
                return createError("The 'parameters' field must be an array of objects.");
            }
            if(arguments.contains("parameters"))
            {
                for(auto const& parameterJson : arguments.at("parameters"))
                {
                    if(!parameterJson.is_object())
                    {
                        return createError("The 'parameters' field must contain an array of objects.");
                    }
                    if(!parameterJson.contains("key") || !parameterJson.at("key").is_string())
                    {
                        return createError("The 'key' field is required and must be a string.");
                    }
                    if(!parameterJson.contains("value") || !parameterJson.at("value").is_number())
                    {
                        return createError("The 'value' field is required and must be a number.");
                    }
                }
            }
            auto const& tracks = arguments.at("tracks");
            auto const& defaultPlugin = arguments.contains("plugin") ? arguments.at("plugin") : nlohmann::json::object();
            auto const defaultGroup = juce::String(arguments.value("groupIdentifier", std::string{}));
            auto const& defaultParameters = arguments.contains("parameters") ? arguments.at("parameters") : nlohmann::json::array();
            juce::StringArray results;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const references = documentDir.sanitize(NotificationType::synchronous);
            // Default group fallbacks
            auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
            if(!layout.empty())
            {
                ScopedDocumentAction actionGuard(documentDir);
                std::set<juce::String> trackIdentifiers;
                nlohmann::json created = nlohmann::json::array();
                for(auto const& trackJson : tracks)
                {
                    if(!trackJson.is_object())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'tracks' argument is required and must be an array of objects.");
                    }
                    if(trackJson.contains("plugin") && !trackJson.at("plugin").is_object())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'plugin' field must be an object.");
                    }
                    if(trackJson.contains("groupIdentifier") && !trackJson.at("groupIdentifier").is_string())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'groupIdentifier' field must be a string.");
                    }
                    auto const& plugin = trackJson.contains("plugin") ? trackJson.at("plugin") : defaultPlugin;
                    if(!trackJson.contains("plugin") && defaultPlugin.empty())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'plugin' field is required either at the top-level or per track.");
                    }
                    if(!plugin.contains("identifier") || !plugin.at("identifier").is_string() || !plugin.contains("feature") || !plugin.at("feature").is_string())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'plugin' field must contain the 'identifier' and 'feature' string fields.");
                    }
                    auto const pluginIdentifier = plugin.at("identifier").get<std::string>();
                    auto const pluginFeature = plugin.at("feature").get<std::string>();
                    if(trackJson.contains("name") && !trackJson.at("name").is_string())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'name' field must be a string.");
                    }
                    if(trackJson.contains("parameters") && !trackJson.at("parameters").is_array())
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError("The 'parameters' field must be an array of objects.");
                    }
                    if(pluginIdentifier.find(":") == std::string::npos)
                    {
                        documentDir.endAction(ActionState::abort);
                        return createError(R"(The 'plugin' object must contain an 'identifier' field in the form 'maker:plugin' and a feature field in the form 'feature', as defined by the tool 'get_installed_vamp_plugins_list', for instance {"identifier":"ircamcrepe:crepe", "feature":"pitch"}. Consider calling the tool 'get_installed_vamp_plugins_list' first to get the correct values.)");
                    }
                    Plugin::Key pluginKey;
                    pluginKey.identifier = pluginIdentifier;
                    pluginKey.feature = pluginFeature;
                    auto const group = trackJson.contains("groupIdentifier")
                                           ? juce::String(trackJson.at("groupIdentifier").get<std::string>())
                                           : (defaultGroup.isNotEmpty() ? defaultGroup : juce::String(layout.front()));
                    auto const groupId = references.count(group) > 0_z ? references.at(group) : group;
                    auto const position = trackJson.value("position", 0_z);
                    auto const [result, identifier] = Tools::addPluginTrack(groupId, position, pluginKey);
                    if(result.failed())
                    {
                        response["isError"] = true;
                        results.add(juce::String(R"(Failed to create track in group "GROUPID" with plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"} due to: REASON Ensure the identifier and feature values correspond to the values returned by the tool 'get_installed_vamp_plugins_list' and that the group exist. Consider calling the tool 'get_installed_vamp_plugins_list' to get the correct plugin identifier and feature, and calling the tool 'get_document_state' to get the correct group identifiers.)").replace("GROUPID", groupId).replace("PLUGINID", pluginIdentifier).replace("PLUGINFEATURE", pluginFeature).replace("REASON", result.getErrorMessage()));
                        break;
                    }
                    if(trackJson.contains("name"))
                    {
                        auto const name = juce::String(trackJson.at("name").get<std::string>());
                        if(Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                        {
                            auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                            trackAcsr.setAttr<Track::AttrType::name>(name, NotificationType::synchronous);
                        }
                    }
                    auto const& trackParameters = trackJson.contains("parameters") ? trackJson.at("parameters") : nlohmann::json::array();
                    if(!defaultParameters.empty() || !trackParameters.empty())
                    {
                        if(!Document::Tools::hasTrackAcsr(documentAcsr, identifier))
                        {
                            response["isError"] = true;
                            results.add(juce::String(R"(The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                            break;
                        }

                        auto& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
                        auto const& description = trackAcsr.getAttr<Track::AttrType::description>();
                        auto state = trackAcsr.getAttr<Track::AttrType::state>();

                        // Apply global defaults first; unsupported parameters are ignored for this track.
                        for(auto const& parameterJson : defaultParameters)
                        {
                            if(!parameterJson.is_object())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'parameters' field must contain an array of objects.");
                            }
                            if(!parameterJson.contains("key") || !parameterJson.at("key").is_string())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'key' field is required and must be a string.");
                            }
                            if(!parameterJson.contains("value") || !parameterJson.at("value").is_number())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'value' field is required and must be a number.");
                            }

                            auto const parameter = parameterJson.at("key").get<std::string>();
                            auto const value = parameterJson.at("value").get<float>();
                            auto const paramIt = std::find_if(description.parameters.cbegin(), description.parameters.cend(),
                                                              [&parameter](auto const& param)
                                                              {
                                                                  return param.identifier == parameter;
                                                              });
                            if(paramIt != description.parameters.cend())
                            {
                                auto const clampedValue = std::clamp(value, paramIt->minValue, paramIt->maxValue);
                                state.parameters[parameter] = clampedValue;
                            }
                        }

                        auto hasParameterError = false;
                        // Apply per-track parameters on top of globals; unknown keys are errors.
                        for(auto const& parameterJson : trackParameters)
                        {
                            if(!parameterJson.is_object())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'parameters' field must contain an array of objects.");
                            }
                            if(!parameterJson.contains("key") || !parameterJson.at("key").is_string())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'key' field is required and must be a string.");
                            }
                            if(!parameterJson.contains("value") || !parameterJson.at("value").is_number())
                            {
                                documentDir.endAction(ActionState::abort);
                                return createError("The 'value' field is required and must be a number.");
                            }

                            auto const parameter = parameterJson.at("key").get<std::string>();
                            auto const value = parameterJson.at("value").get<float>();
                            auto const paramIt = std::find_if(description.parameters.cbegin(), description.parameters.cend(),
                                                              [&parameter](auto const& param)
                                                              {
                                                                  return param.identifier == parameter;
                                                              });
                            if(paramIt == description.parameters.cend())
                            {
                                response["isError"] = true;
                                results.add(juce::String(R"(The parameter "PARAM" of the track "TRACKID" doesn't exist.)").replace("TRACKID", identifier).replace("PARAM", parameter));
                                hasParameterError = true;
                                break;
                            }
                            else
                            {
                                auto const clampedValue = std::clamp(value, paramIt->minValue, paramIt->maxValue);
                                state.parameters[parameter] = clampedValue;
                            }
                        }

                        if(hasParameterError)
                        {
                            break;
                        }

                        trackAcsr.setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                    }
                    trackIdentifiers.insert(identifier);
                    created.push_back(identifier);
                    if(trackJson.contains("name"))
                    {
                        auto const name = juce::String(trackJson.at("name").get<std::string>());
                        results.add(juce::String(R"(Created track "TRACKID" named "TRACKNAME" with plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"}.)")
                                        .replace("TRACKID", identifier)
                                        .replace("TRACKNAME", name)
                                        .replace("PLUGINID", pluginIdentifier)
                                        .replace("PLUGINFEATURE", pluginFeature));
                    }
                    else
                    {
                        results.add(juce::String(R"(Created track "TRACKID" with plugin {"identifier":"PLUGINID", "feature":"PLUGINFEATURE"}.)")
                                        .replace("TRACKID", identifier)
                                        .replace("PLUGINID", pluginIdentifier)
                                        .replace("PLUGINFEATURE", pluginFeature));
                    }
                }
                if(!response.at("isError").get<bool>())
                {
                    actionGuard.commit(juce::translate("Create tracks (Neuralyzer)"));
                }
                else
                {
                    actionGuard.abort();
                }
                nlohmann::json content;
                content["type"] = "text";
                nlohmann::json payload;
                payload["created"] = created;
                payload["messages"] = results.joinIntoString("\n");
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
        if(toolName == "create_tracks_with_files")
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
            if(arguments.contains("groupIdentifier") && !arguments.at("groupIdentifier").is_string())
            {
                return createError("The 'groupIdentifier' field must be a string.");
            }
            auto const& tracks = arguments.at("tracks");
            auto const defaultGroupIdentifier = juce::String(arguments.value("groupIdentifier", std::string{}));
            juce::StringArray results;
            auto& documentAcsr = Instance::get().getDocumentAccessor();
            auto& documentDir = Instance::get().getDocumentDirector();
            auto const references = documentDir.sanitize(NotificationType::synchronous);
            auto const& layout = documentAcsr.getAttr<Document::AttrType::layout>();
            if(!layout.empty())
            {
                ScopedDocumentAction actionGuard(documentDir);
                std::set<juce::String> trackIdentifiers;
                nlohmann::json created = nlohmann::json::array();
                for(auto const& trackJson : tracks)
                {
                    if(!trackJson.is_object())
                    {
                        return createError("The 'tracks' argument is required and must be an array of objects.");
                    }
                    if(!trackJson.contains("file") || !trackJson.at("file").is_string())
                    {
                        return createError("The 'file' field is required and must be a string.");
                    }
                    if(trackJson.contains("groupIdentifier") && !trackJson.at("groupIdentifier").is_string())
                    {
                        return createError("The 'groupIdentifier' field must be a string.");
                    }

                    auto const file = juce::File(trackJson.at("file").get<std::string>());
                    if(!file.existsAsFile() || !file.hasReadAccess())
                    {
                        return createError(juce::String("The file 'FILEPATH' does not exist or is not readable.").replace("FILEPATH", file.getFullPathName()));
                    }

                    auto const group = trackJson.contains("groupIdentifier")
                                           ? juce::String(trackJson.at("groupIdentifier").get<std::string>())
                                           : (defaultGroupIdentifier.isNotEmpty() ? defaultGroupIdentifier : juce::String(layout.front()));
                    auto const groupId = references.count(group) > 0_z ? references.at(group) : group;
                    auto position = size_t{0};
                    if(trackJson.contains("position"))
                    {
                        if(!trackJson.at("position").is_number_integer())
                        {
                            return createError("The 'position' field must be an integer.");
                        }
                        position = trackJson.at("position").get<size_t>();
                    }
                    else if(Document::Tools::hasGroupAcsr(documentAcsr, groupId))
                    {
                        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
                        position = groupAcsr.getAttr<Group::AttrType::layout>().size();
                    }

                    auto const [result, identifier] = Tools::addFileTrack(groupId, position, file);
                    if(result.failed())
                    {
                        response["isError"] = true;
                        results.add(juce::String(R"(Failed to create track in group "GROUPID" from file "FILEPATH": REASON Ensure the file path is valid and readable, and that the group exists. Consider calling the tool 'get_document_state' to get the correct group identifiers.)").replace("GROUPID", groupId).replace("FILEPATH", file.getFullPathName()).replace("REASON", result.getErrorMessage()));
                        break;
                    }
                    trackIdentifiers.insert(identifier);
                    created.push_back(identifier);
                    results.add(juce::String(R"(Created track "TRACKID" from file "FILEPATH".)").replace("TRACKID", identifier).replace("FILEPATH", file.getFullPathName()));
                }
                if(!response.at("isError").get<bool>())
                {
                    actionGuard.commit(juce::translate("Create tracks from files (Neuralyzer)"));
                }
                else
                {
                    actionGuard.abort();
                }
                nlohmann::json content;
                content["type"] = "text";
                nlohmann::json payload;
                payload["created"] = created;
                payload["messages"] = results.joinIntoString("\n");
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
            ScopedDocumentAction actionGuard(documentDir);
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
                        results.add(juce::String(R"(The track "TRACKID" could not be removed: REASON.)").replace("TRACKID", identifier).replace("REASON", result.getErrorMessage()));
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
                    results.add(juce::String(R"("The track "TRACKID" doesn't exist.)").replace("TRACKID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Remove tracks (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
            }
            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["removed"] = removed;
            payload["messages"] = results.joinIntoString("\n");
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
            ScopedDocumentAction actionGuard(documentDir);
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
                    results.add(juce::String(R"(Failed to create group "NAME": REASON.)").replace("NAME", name).replace("REASON", result.getErrorMessage()));
                    break;
                }
                auto& documentAcsr = Instance::get().getDocumentAccessor();
                auto& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, identifier);
                if(!name.isEmpty())
                {
                    groupAcsr.setAttr<Group::AttrType::name>(name, NotificationType::synchronous);
                }
                created.push_back(identifier);
                results.add(juce::String(R"(Created group "GROUPID" with name "NAME".)").replace("GROUPID", identifier).replace("NAME", groupAcsr.getAttr<Group::AttrType::name>()));
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Create groups (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
            }
            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["created"] = created;
            payload["messages"] = results.joinIntoString("\n");
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
            ScopedDocumentAction actionGuard(documentDir);
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
                        results.add(juce::String(R"(The group "GROUPID" could not be removed: REASON.)").replace("GROUPID", identifier).replace("REASON", result.getErrorMessage()));
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
                    results.add(juce::String(R"(The group "GROUPID" doesn't exist.)").replace("GROUPID", identifier));
                    break;
                }
            }
            if(!response.at("isError").get<bool>())
            {
                actionGuard.commit(juce::translate("Remove groups (Neuralyzer)"));
            }
            else
            {
                actionGuard.abort();
            }
            nlohmann::json content;
            content["type"] = "text";
            nlohmann::json payload;
            payload["removed"] = removed;
            payload["messages"] = results.joinIntoString("\n");
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
                auto const quoted = stringToColour(R"("#aabbccdd")");
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

nlohmann::json Application::Neuralyzer::Mcp::Dispatcher::callMethod(nlohmann::json const& request, DynamicToolMethods const& methods)
{
    if(!request.contains("method") || !request.at("method").is_string())
    {
        return createError("The 'method' field is required and must be an object.");
    }
    auto const method = request.at("method").get<std::string>();
    if(method == "initialize")
    {
        auto result = nlohmann::json::object();
        result["protocolVersion"] = "2025-11-25";
        result["capabilities"]["tools"] = nlohmann::json::object();
        result["capabilities"]["resources"] = nlohmann::json::object();
        result["serverInfo"]["name"] = "Partiels MCP Host";
        result["serverInfo"]["version"] = "0.1.0";
        result["instructions"] = juce::String::fromUTF8(AnlNeuralyzerData::Instructions_md, AnlNeuralyzerData::Instructions_mdSize);
        return result;
    }
    if(method == "notifications/initialized")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP initialized notification");
        return {};
    }
    if(method == "tools/list")
    {
        MiscDebug("Application::Neuralyzer::Mcp::Dispatcher", "Received MCP tools/list");
        auto list = nlohmann::json::parse(AnlNeuralyzerData::ToolsList_json);
        // Remove outputSchema from list
        for(auto& tool : list["tools"])
        {
            tool.erase("outputSchema");
        }
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
        return readResource(request);
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
            return createError("The 'params' field is required and must be an object.");
        }
        auto const methodParams = request.at("params");
        if(!methodParams.contains("name") || !methodParams.at("name").is_string())
        {
            return createError("The 'name' field is required and must be a string.");
        }

        auto const toolName = methodParams.at("name").get<std::string>();
        // Global methods to call on this threa
        if(toolName == "search_docs")
        {
            if(methods.searchDocsFn == nullptr)
            {
                return createError("The list docs callback is unavailable.");
            }
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("query") || !arguments.at("query").is_string())
            {
                return createError("The 'query' argument is required and must a string.");
            }
            if(arguments.contains("maxNumDocuments") && !arguments.at("maxNumDocuments").is_number())
            {
                return createError("The 'maxNumDocuments' argument must be a postive integer.");
            }

            auto const query = arguments.at("query").get<std::string>();
            auto const maxNumDocuments = arguments.contains("maxNumDocuments") ? arguments.at("maxNumDocuments").get<int>() : 10;
            if(maxNumDocuments <= 0)
            {
                return createError("The 'maxNumDocuments' argument must be a postive integer.");
            }
            return methods.searchDocsFn(query, static_cast<size_t>(maxNumDocuments));
        }
        if(toolName == "load_docs")
        {
            if(methods.loadDocsFn == nullptr)
            {
                return createError("The load docs callback is unavailable.");
            }
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("ids") || !arguments.at("ids").is_array() || arguments.at("ids").empty())
            {
                return createError("The 'ids' argument is required and must an array of strings.");
            }
            for(auto const& id : arguments.at("ids"))
            {
                if(!id.is_string())
                {
                    return createError("The 'ids' argument is required and must an array of strings.");
                }
            }
            auto const ids = arguments.at("ids").get<std::vector<std::string>>();
            return methods.loadDocsFn(ids);
        }
        if(toolName == "read_files")
        {
            if(methods.readFilesFn == nullptr)
            {
                return createError("The file reader callback is unavailable.");
            }
            if(!methodParams.contains("arguments") || !methodParams.at("arguments").is_object())
            {
                return createError("The 'arguments' field is required and must be an object.");
            }
            auto const& arguments = methodParams.at("arguments");
            if(!arguments.contains("files") || !arguments.at("files").is_array() || arguments.at("files").empty())
            {
                return createError("The 'files' argument is required and must be an array of strings.");
            }
            for(auto const& file : arguments.at("files"))
            {
                if(!file.is_string())
                {
                    return createError("The 'files' argument is required and must an array of strings.");
                }
            }
            auto const files = arguments.at("files").get<std::vector<std::string>>();
            return methods.readFilesFn(files);
        }

        return juce::MessageManager::callSync([&]()
                                              {
                                                  try
                                                  {
                                                      return performToolsCall(methodParams);
                                                  }
                                                  catch(std::exception const& e)
                                                  {
                                                      return createError(e.what());
                                                  }
                                              });
    }
    return createError("Unknown method: " + method);
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
        if(resource.contains("name") && resource.at("name").is_string() && resource.contains("uri") && resource.at("uri").is_string())
        {
            nlohmann::json readRequest;
            readRequest["method"] = "resources/read";
            readRequest["params"]["uri"] = resource.value("uri", std::string{});
            auto const contentResult = callMethod(readRequest);
            MiscWeakAssert(contentResult.contains("contents") && contentResult.at("contents").is_array());
            if(contentResult.contains("contents") && contentResult.at("contents").is_array())
            {
                auto const name = resource.at("name").get<std::string>();
                for(auto const& content : contentResult.at("contents"))
                {
                    MiscWeakAssert(content.contains("text") && content.at("text").is_string());
                    if(content.contains("text") && content.at("text").is_string())
                    {
                        resources.push_back(std::make_pair(name, content.at("text").get<std::string>()));
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
    auto const response = callMethod(request);
    MiscWeakAssert(response.contains("tools") && response.at("tools").is_array());
    if(!response.contains("tools") || !response.at("tools").is_array())
    {
        return {};
    }
    auto const isSupportedTool = [&](std::string const& name)
    {
        if(name == "read_files")
        {
            return methods.readFilesFn != nullptr;
        }
        if(name == "search_docs")
        {
            return methods.searchDocsFn != nullptr;
        }
        if(name == "load_docs")
        {
            return methods.loadDocsFn != nullptr;
        }
        return true;
    };
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
            if(isSupportedTool(ctool.name))
            {
                ctools.push_back(std::move(ctool));
            }
        }
    }
    return ctools;
}

std::pair<bool, std::vector<std::string>> Application::Neuralyzer::Mcp::Dispatcher::callTools(DynamicToolMethods const& methods, std::vector<common_chat_tool_call> const& toolCalls)
{
    auto results = std::make_pair(true, std::vector<std::string>{});
    auto const unquote = [](std::string const& str) -> std::string
    {
        return juce::String(str).replace(R"(\")", R"(")").toStdString();
    };
    for(auto const& toolCall : toolCalls)
    {
        MiscDebug("Application::Neuralyzer::Tools", juce::String("Calling tool: ") + toolCall.name + " (ID: " + toolCall.id + ")");

        nlohmann::json request;
        request["method"] = "tools/call";
        request["params"]["name"] = toolCall.name;
        request["params"]["arguments"] = toolCall.arguments.empty() ? nlohmann::json::object() : nlohmann::json::parse(toolCall.arguments);

        std::string message;
        message += "[Tool Call ID: " + toolCall.id + "]\n";
        message += "Tool: " + toolCall.name + "\n";

        try
        {
            auto const dispatcherResponse = callMethod(request, methods);
            if(dispatcherResponse.contains("error"))
            {
                results.first = false;
                message += "Status: ERROR\n";
                if(dispatcherResponse.at("error").is_object())
                {
                    auto const& error = dispatcherResponse.at("error");
                    if(error.contains("message") && error.at("message").is_string())
                    {
                        message += " Message: " + unquote(error.at("message").get<std::string>()) + "\n";
                    }
                    if(error.contains("code") && error.at("code").is_number())
                    {
                        message += " Code: " + std::to_string(error.at("code").get<int>()) + "\n";
                    }
                }
                else
                {
                    message += "Message: " + unquote(dispatcherResponse.at("error").dump());
                }
            }
            else
            {
                auto content = dispatcherResponse.value("content", nlohmann::json::array());
                auto const text = unquote(content[0].value("text", "(empty)"));
                if(dispatcherResponse.value("isError", false))
                {
                    results.first = false;
                    message += "Status: FAILED\n";
                    message += "Message: " + text;
                }
                else
                {
                    message += "Status: SUCCESS\n";
                    message += "Result: " + text;
                }
            }
        }
        catch(std::exception const& e)
        {
            results.first = false;
            message += "Status: ERROR\n";
            message += "Message: Exception during execution: " + std::string(e.what());
        }
        catch(...)
        {
            results.first = false;
            message += "Status: ERROR\n";
            message += "Message: Exception during execution: Unknown";
        }
        results.second.push_back(message);
    }
    return results;
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
        auto const result = mDispatcher.callMethod(request);
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
                break;
            case AttrType::mcpForClaudeApp:
            case AttrType::mcpForCopilotApp:
            {
                auto const mcpForClaudeApp = acsr.getAttr<AttrType::mcpForClaudeApp>();
                auto const mcpForCopilotApp = acsr.getAttr<AttrType::mcpForCopilotApp>();
                if(mcpForClaudeApp || mcpForCopilotApp)
                {
                    if(!beginWaitingForSocket(Host::defaultPort))
                    {
                        MiscDebug("Application::Instance", "Failed to start MCP server on port " + juce::String(Host::defaultPort));
                    }
                }
                else
                {
                    stop();
                }

                setClaudeApplicationEnabled(mcpForClaudeApp);
                setCopilotApplicationEnabled(mcpForCopilotApp);
                break;
            }
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::Neuralyzer::Mcp::Server::~Server()
{
    setClaudeApplicationEnabled(false);
    setCopilotApplicationEnabled(false);
    mAccessor.removeListener(mListener);
    stop();
}

juce::InterprocessConnection* Application::Neuralyzer::Mcp::Server::createConnectionObject()
{
    mConnections = std::make_unique<Connection>(mDispatcher);
    return mConnections.get();
}

juce::String Application::Neuralyzer::Mcp::Server::getPartielsExecutablePath()
{
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getFullPathName();
}

bool Application::Neuralyzer::Mcp::Server::setClaudeApplicationEnabled([[maybe_unused]] bool enabled)
{
#ifdef JUCE_MAC
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support").getChildFile("Claude");
#else
    auto const directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Claude");
#endif
    if(directory.createDirectory().failed())
    {
        return false;
    }
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
        json["mcpServers"]["Partiels"] = nlohmann::json{{"command", getPartielsExecutablePath().toStdString()}, {"args", nlohmann::json::array({Neuralyzer::Mcp::Host::defaultArg})}, {"timeout", 300000}};
    }
    else if(json.contains("mcpServers"))
    {
        json["mcpServers"].erase("Partiels");
    }
    juce::String const text = json.dump(2);
    auto const result = file.replaceWithText(text);
    MiscWeakAssert(result);
    return result;
}

bool Application::Neuralyzer::Mcp::Server::setCopilotApplicationEnabled([[maybe_unused]] bool enabled)
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
    if(directory.createDirectory().failed())
    {
        return false;
    }
    auto const file = directory.getChildFile("mcp.json");

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
        json["servers"]["partiels"] = nlohmann::json{{"type", "stdio"}, {"command", getPartielsExecutablePath().toStdString()}, {"args", nlohmann::json::array({Neuralyzer::Mcp::Host::defaultArg})}};
    }
    else if(json.contains("servers"))
    {
        json["servers"].erase("partiels");
    }

    auto const result = file.replaceWithText(juce::String(json.dump(2)));
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
                auto const text = message.toString();
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
            if(method == "shutdown")
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
