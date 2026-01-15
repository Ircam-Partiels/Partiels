#include "AnlApplicationNeuralyzerMcpHandlers.h"
#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerMcpDispatcher.h"
#include "AnlApplicationNeuralyzerMcpOperations.h"

ANALYSE_FILE_BEGIN

namespace Application::Neuralyzer::Mcp
{
    // Gets a required argument or throws an ExecutionError with the error message if the argument is missing or has an invalid type
    template <typename T>
    static T getArgument(nlohmann::json const& arguments, std::string const& name, std::string const& error)
    {
        if(!arguments.is_object() || !arguments.contains(name))
        {
            throw ExecutionError(error);
        }
        try
        {
            return arguments.at(name).get<T>();
        }
        catch(nlohmann::json::exception const&)
        {
            throw ExecutionError(error);
        }
    }

    // Calls the function on the message thread and returns its result (the exceptions are forwarded to the caller)
    // The call is abandoned if the abort flag is set or if the current thread should exit (for example, when the
    // message thread waits for this thread to stop), so the threads never wait for each other indefinitely
    template <typename Function>
    static auto callOnMessageThread(std::atomic<bool> const& shouldAbort, Function&& function) -> decltype(function())
    {
        using result_t = decltype(function());
        if(juce::MessageManager::existsAndIsCurrentThread())
        {
            return function();
        }

        using value_t = std::conditional_t<std::is_void_v<result_t>, std::monostate, result_t>;
        struct State
        {
            std::mutex mutex;
            bool isAbandoned{false};
            std::promise<value_t> promise;
        };
        auto state = std::make_shared<State>();
        auto future = state->promise.get_future();
        // The function is only called if the call has not been abandoned so the references remain valid
        auto const posted = juce::MessageManager::callAsync([state, &function]()
                                                            {
                                                                std::unique_lock<std::mutex> lock(state->mutex);
                                                                if(state->isAbandoned)
                                                                {
                                                                    return;
                                                                }
                                                                try
                                                                {
                                                                    if constexpr(std::is_void_v<result_t>)
                                                                    {
                                                                        function();
                                                                        state->promise.set_value(std::monostate{});
                                                                    }
                                                                    else
                                                                    {
                                                                        state->promise.set_value(function());
                                                                    }
                                                                }
                                                                catch(...)
                                                                {
                                                                    state->promise.set_exception(std::current_exception());
                                                                }
                                                            });
        if(!posted)
        {
            throw ExecutionError("The message thread is unavailable.");
        }
        while(future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready)
        {
            if(shouldAbort.load() || juce::Thread::currentThreadShouldExit())
            {
                std::unique_lock<std::mutex> lock(state->mutex);
                if(future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    state->isAbandoned = true;
                    throw ExecutionError("The operation has been aborted.");
                }
            }
        }
        if constexpr(std::is_void_v<result_t>)
        {
            [[maybe_unused]] auto const value = future.get();
        }
        else
        {
            return future.get();
        }
    }

    // Converts a juce::Colour to a string in the format "#RRGGBBAA"
    static std::string colourToString(juce::Colour const& colour)
    {
        auto const value = (static_cast<uint32_t>(colour.getRed()) << 24u | static_cast<uint32_t>(colour.getGreen()) << 16u | static_cast<uint32_t>(colour.getBlue()) << 8u | static_cast<uint32_t>(colour.getAlpha()));
        std::array<char, 10> buf;
        std::snprintf(buf.data(), buf.size(), "#%08x", value);
        return std::string(buf.data());
    }

    // Converts a string in the format "#RRGGBB" or "#RRGGBBAA" to a juce::Colour
    static std::optional<juce::Colour> stringToColour(std::string str)
    {
        str = juce::String(str).unquoted().toStdString();
        if((str.size() != 7_z && str.size() != 9_z) || str.front() != '#' || !std::all_of(std::next(str.cbegin()), str.cend(), [](auto const c)
                                                                                          {
                                                                                              return std::isxdigit(static_cast<unsigned char>(c)) != 0;
                                                                                          }))
        {
            return {};
        }
        auto const value = static_cast<uint32_t>(std::stoul(str.substr(1_z), nullptr, 16));
        if(str.size() == 7_z)
        {
            return juce::Colour(static_cast<juce::uint8>((value >> 16) & 0xFF), static_cast<juce::uint8>((value >> 8) & 0xFF), static_cast<juce::uint8>(value & 0xFF), static_cast<juce::uint8>(255));
        }
        return juce::Colour(static_cast<juce::uint8>((value >> 24) & 0xFF), static_cast<juce::uint8>((value >> 16) & 0xFF), static_cast<juce::uint8>((value >> 8) & 0xFF), static_cast<juce::uint8>(value & 0xFF));
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
                expect(!stringToColour("").has_value());
                expect(!stringToColour("#").has_value());
                expect(!stringToColour("#zzzzzz").has_value());
                expect(!stringToColour("#ff00").has_value());

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

    // Creates a tool response with a text content
    static nlohmann::json createTextResponse(std::string const& text, bool isError)
    {
        nlohmann::json content;
        content["type"] = "text";
        content["text"] = text;
        nlohmann::json response;
        response["isError"] = isError;
        response["content"] = nlohmann::json::array({content});
        return response;
    }

    // Creates a tool response with a JSON object as structured content and as text content (for the clients that don't support structured content)
    static nlohmann::json createJsonResponse(nlohmann::json const& payload, bool isError)
    {
        MiscWeakAssert(payload.is_object());
        auto response = createTextResponse(payload.dump(), isError);
        response["structuredContent"] = payload;
        return response;
    }

} // namespace Application::Neuralyzer::Mcp

// JSON serialization of the results of the operations (the format of the responses of the tools)
namespace Application::Neuralyzer::Mcp::Operations
{
    static void to_json(nlohmann::json& j, TrackState const& track)
    {
        j["identifier"] = track.identifier;
        j["name"] = track.name;
    }

    static void to_json(nlohmann::json& j, NewTrackState const& track)
    {
        j["identifier"] = track.identifier;
        j["name"] = track.name;
        if(!track.linkedTracks.empty())
        {
            j["linkedTracks"] = track.linkedTracks;
        }
    }

    static void to_json(nlohmann::json& j, LinkedTracks const& linkedTracks)
    {
        j["count"] = linkedTracks.count;
        j["minCount"] = linkedTracks.minCount;
        j["maxCount"] = linkedTracks.maxCount;
        j["defaultCount"] = linkedTracks.defaultCount;
        j["tracks"] = linkedTracks.tracks;
    }

    static void to_json(nlohmann::json& j, GroupState const& group)
    {
        j["identifier"] = group.identifier;
        j["name"] = group.name;
        j["tracks"] = group.tracks;
    }

    static void to_json(nlohmann::json& j, ZoomRanges const& ranges)
    {
        j["visibleRange"] = ranges.visibleRange;
        j["globalRange"] = ranges.globalRange;
    }

    static void to_json(nlohmann::json& j, TrackPropertiesState const& state)
    {
        j = nlohmann::json::object();
        if(state.name.has_value())
        {
            j["name"] = state.name.value();
        }
        if(state.visibleInGroup.has_value())
        {
            j["visibleInGroup"] = state.visibleInGroup.value();
        }
        if(state.extraThresholds.has_value())
        {
            j["extraThresholds"] = state.extraThresholds.value();
        }
        if(state.verticalZoom.has_value())
        {
            j["verticalZoom"] = state.verticalZoom.value();
        }
        if(state.valueRange.has_value())
        {
            j["valueRange"] = state.valueRange.value();
        }
    }

    static void to_json(nlohmann::json& j, GroupPropertiesState const& state)
    {
        j = nlohmann::json::object();
        if(state.name.has_value())
        {
            j["name"] = state.name.value();
        }
        if(state.layout.has_value())
        {
            j["layout"] = state.layout.value();
        }
        if(state.verticalZoom.has_value())
        {
            j["verticalZoom"] = state.verticalZoom.value();
        }
    }

    static void to_json(nlohmann::json& j, SelectedItems const& items)
    {
        j["groups"] = items.groups;
        j["tracks"] = items.tracks;
    }

    static void to_json(nlohmann::json& j, AudioFileInfo const& info)
    {
        j["formatName"] = info.formatName;
        j["sampleRate"] = info.sampleRate;
        j["bitsPerSample"] = info.bitsPerSample;
        j["usesFloatingPointData"] = info.usesFloatingPointData;
        j["lengthInSamples"] = static_cast<int64_t>(info.lengthInSamples);
        j["durationSeconds"] = info.sampleRate > 0.0 ? static_cast<double>(info.lengthInSamples) / info.sampleRate : 0.0;
        j["numChannels"] = info.numChannels;
        j["metadata"] = info.metadata.size() > 0 ? nlohmann::json(info.metadata) : nlohmann::json::object();
    }

    static void to_json(nlohmann::json& j, Statistics const& statistics)
    {
        j["min"] = statistics.min;
        j["max"] = statistics.max;
        j["median"] = statistics.median;
        j["mean"] = statistics.mean;
        j["variance"] = statistics.variance;
        j["skewness"] = statistics.skewness;
    }

    static void to_json(nlohmann::json& j, ExtraValuesSummary const& summary)
    {
        j["index"] = summary.index;
        j["identifier"] = summary.identifier;
        j["name"] = summary.name;
        j["unit"] = summary.unit;
        j["stats"] = summary.statistics;
    }

    static void to_json(nlohmann::json& j, ResultsRangeSummary const& summary)
    {
        if(summary.timeRange.has_value())
        {
            j["timeRange"] = nlohmann::json::array({summary.timeRange->getStart(), summary.timeRange->getEnd()});
        }
        j["count"] = summary.count;
        if(summary.breakCount.has_value())
        {
            j["breakCount"] = summary.breakCount.value();
        }
        if(summary.values.has_value())
        {
            j["value"] = summary.values.value();
        }
        j["extraValues"] = summary.extraValues;
    }

    static void to_json(nlohmann::json& j, ResultsChannelSummary const& summary)
    {
        // Without time range, the summary of the channel contains directly the summary of all the results
        if(summary.ranges.size() == 1_z && !summary.ranges.front().timeRange.has_value())
        {
            j = summary.ranges.front();
        }
        else
        {
            j["timeRanges"] = summary.ranges;
        }
        j["channel"] = summary.channel;
    }

    static void to_json(nlohmann::json& j, ResultsSummary const& summary)
    {
        j["type"] = magic_enum::enum_name(summary.type);
        j["channels"] = summary.channels;
    }

    static void to_json(nlohmann::json& j, CompatibleInput const& input)
    {
        j["inputIdentifier"] = input.inputIdentifier;
        j["compatibleIdentifiers"] = input.compatibleIdentifiers;
    }

    static void to_json(nlohmann::json& j, TrackGraphics const& graphics)
    {
        j = nlohmann::json::object();
        auto const set = [&](char const* name, auto const& property)
        {
            if(!property.has_value())
            {
                return;
            }
            using value_t = std::decay_t<decltype(property.value())>;
            if constexpr(std::is_same_v<value_t, juce::Colour>)
            {
                j[name] = colourToString(property.value());
            }
            else if constexpr(std::is_enum_v<value_t>)
            {
                j[name] = magic_enum::enum_name(property.value());
            }
            else
            {
                j[name] = property.value();
            }
        };
        set("colorMap", graphics.colourMap);
        set("colorBackground", graphics.backgroundColour);
        set("colorForeground", graphics.foregroundColour);
        set("colorDuration", graphics.durationColour);
        set("colorText", graphics.textColour);
        set("colorShadow", graphics.shadowColour);
        set("fontName", graphics.fontName);
        set("fontStyle", graphics.fontStyle);
        set("fontSize", graphics.fontSize);
        set("lineWidth", graphics.lineWidth);
        set("unit", graphics.unit);
        set("labelLayoutPosition", graphics.labelLayoutPosition);
        set("labelLayoutJustification", graphics.labelLayoutJustification);
        set("markerStyle", graphics.markerStyle);
    }

    class JsonUnitTest
    : public juce::UnitTest
    {
    public:
        JsonUnitTest()
        : juce::UnitTest("Mcp Operations Json", "Application")
        {
        }

        ~JsonUnitTest() override = default;

        void runTest() override
        {
            beginTest("document state");
            {
                expect(nlohmann::json(std::vector<GroupState>{}) == nlohmann::json::array());
                GroupState const group{"g1", "Group", {{"t1", "Track"}}};
                expect(nlohmann::json(group) == nlohmann::json::parse(R"({"identifier":"g1","name":"Group","tracks":[{"identifier":"t1","name":"Track"}]})"));
            }

            beginTest("zoom ranges");
            {
                ZoomRanges const ranges{{1.0, 2.0}, {0.0, 10.0}};
                expect(nlohmann::json(ranges) == nlohmann::json::parse(R"({"visibleRange":{"start":1.0,"end":2.0},"globalRange":{"start":0.0,"end":10.0}})"));
            }

            beginTest("selected items");
            {
                expect(nlohmann::json(SelectedItems{}) == nlohmann::json::parse(R"({"groups":[],"tracks":[]})"));
                expect(nlohmann::json(SelectedItems{{"g1"}, {"t1", "t2"}}) == nlohmann::json::parse(R"({"groups":["g1"],"tracks":["t1","t2"]})"));
            }

            beginTest("audio file info");
            {
                AudioFileInfo info;
                info.formatName = "WAV file";
                info.sampleRate = 48000.0;
                info.bitsPerSample = 24u;
                info.lengthInSamples = 96000;
                info.numChannels = 2u;
                auto json = nlohmann::json(info);
                expect(json.at("metadata") == nlohmann::json::object());
                expectEquals(json.at("durationSeconds").get<double>(), 2.0);
                info.metadata.set("key", "value");
                json = nlohmann::json(info);
                expect(json.at("metadata") == nlohmann::json::parse(R"({"key":"value"})"));
            }
        }
    };

    static JsonUnitTest jsonUnitTest;
} // namespace Application::Neuralyzer::Mcp::Operations

nlohmann::json Application::Neuralyzer::Mcp::createToolErrorResponse(std::string const& message)
{
    return createTextResponse(message, true);
}

namespace Application::Neuralyzer::Mcp
{
    // Gets an optional argument (or the default value if the argument is missing) or throws an ExecutionError with the error message if the argument has an invalid type
    template <typename T>
    static T getOptionalArgument(nlohmann::json const& arguments, std::string const& name, T const& defaultValue, std::string const& error)
    {
        if(!arguments.is_object() || !arguments.contains(name))
        {
            return defaultValue;
        }
        return getArgument<T>(arguments, name, error);
    }

    // Gets an array of objects with an 'identifier' string field as pairs of identifiers and objects in the order of the array
    static std::vector<std::pair<juce::String, nlohmann::json>> getOrderedItemsArgument(nlohmann::json const& arguments, std::string const& name, std::string const& error)
    {
        std::vector<std::pair<juce::String, nlohmann::json>> items;
        for(auto const& item : getArgument<std::vector<nlohmann::json>>(arguments, name, error))
        {
            if(!item.is_object())
            {
                throw ExecutionError(error);
            }
            auto const identifier = getArgument<std::string>(item, "identifier", "The 'identifier' field is required and must be a string.");
            items.emplace_back(identifier, item);
        }
        return items;
    }

    // Gets an array of objects with an 'identifier' string field as a map of identifiers to objects
    static std::map<juce::String, nlohmann::json> getItemsArgument(nlohmann::json const& arguments, std::string const& name, std::string const& error)
    {
        std::map<juce::String, nlohmann::json> items;
        for(auto const& [identifier, item] : getOrderedItemsArgument(arguments, name, error))
        {
            items.insert_or_assign(identifier, item);
        }
        return items;
    }

    static std::string toKey(juce::String const& identifier)
    {
        return identifier.toStdString();
    }

    static std::string toKey(juce::File const& file)
    {
        return file.getFullPathName().toStdString();
    }

    // Converts a value to JSON, the enumerations are converted to their names
    template <typename T>
    static nlohmann::json toJson(T const& value)
    {
        if constexpr(std::is_enum_v<T>)
        {
            return magic_enum::enum_name(value);
        }
        else
        {
            return value;
        }
    }

    // Converts a map with string keys to an object (instead of an array of pairs)
    template <typename T>
    static nlohmann::json toJson(std::map<juce::String, T> const& values)
    {
        auto json = nlohmann::json::object();
        for(auto const& [key, value] : values)
        {
            json[toKey(key)] = toJson(value);
        }
        return json;
    }

    static nlohmann::json toJson(Plugin::Description const& description)
    {
        nlohmann::json json(description);
        // The names of the bins are removed to keep the response lightweight
        if(json.contains("output") && json.at("output").is_object())
        {
            json["output"].erase("binNames");
        }
        // The voices of the outputs are presented as linked tracks (as in the interface) to avoid the confusion with
        // the voices of the audio (singing or speech)
        auto const rename = [&](char const* from, char const* to)
        {
            if(json.contains(from))
            {
                json[to] = std::move(json.at(from));
                json.erase(from);
            }
        };
        rename("voices", "linkedTrackFeatures");
        rename("voiceDescriptor", "linkedTrackCountDescriptor");
        return json;
    }

    // Gets an optional enumeration argument from its name or throws an ExecutionError with the valid names
    template <typename T>
    static std::optional<T> getOptionalEnumArgument(nlohmann::json const& arguments, std::string const& name)
    {
        if(!arguments.contains(name))
        {
            return {};
        }
        juce::StringArray names;
        for(auto const& enumName : magic_enum::enum_names<T>())
        {
            names.add("'" + juce::String(std::string(enumName)) + "'");
        }
        auto const error = "The '" + name + "' field must be one of the following strings: " + names.joinIntoString(", ").toStdString() + ".";
        auto const value = magic_enum::enum_cast<T>(getArgument<std::string>(arguments, name, error));
        if(!value.has_value())
        {
            throw ExecutionError(error);
        }
        return value;
    }

    // The properties of the tracks returned by the tool get_track_properties
    // clang-format off
    enum class TrackProperty
    {
          name
        , type
        , description
        , parameters
        , graphics
        , inputs
        , visibleInGroup
        , extraThresholds
        , verticalZoom
        , valueRange
        , linkedTracks
    };

    // The properties of the groups returned by the tool get_group_properties
    enum class GroupProperty
    {
          name
        , layout
        , verticalZoom
    };

    // The properties of the document returned by the tool get_document_state
    enum class DocumentProperty
    {
          groups
        , timeZoom
        , timeSelection
        , selectedItems
        , audioFileLayout
    };
    // clang-format on

    // Gets the required 'properties' argument (a non-empty array of property names without duplicates) or throws an ExecutionError with the valid names
    template <typename Property>
    static std::vector<Property> getPropertiesArgument(nlohmann::json const& arguments)
    {
        juce::StringArray names;
        for(auto const& enumName : magic_enum::enum_names<Property>())
        {
            names.add("'" + juce::String(std::string(enumName)) + "'");
        }
        auto const error = "The 'properties' field is required and must be a non-empty array of the following strings: " + names.joinIntoString(", ").toStdString() + ".";
        std::vector<Property> properties;
        for(auto const& name : getArgument<std::vector<std::string>>(arguments, "properties", error))
        {
            auto const property = magic_enum::enum_cast<Property>(name);
            if(!property.has_value())
            {
                throw ExecutionError(error);
            }
            if(std::find(properties.cbegin(), properties.cend(), property.value()) == properties.cend())
            {
                properties.push_back(property.value());
            }
        }
        if(properties.empty())
        {
            throw ExecutionError(error);
        }
        return properties;
    }

    // Adds the values of a property to the payload of a tool that gets properties ({"results": {...}, "errors": {...}}):
    // the items that don't exist are errors and the properties that cannot be retrieved for an existing item are
    // reported in its 'propertyErrors'
    template <typename Property, typename Values, typename Function>
    static void addPropertyValues(nlohmann::json& payload, Property property, Values const& values, Function&& exists)
    {
        auto const propertyName = std::string(magic_enum::enum_name(property));
        for(auto const& [identifier, itemResult] : values)
        {
            auto const key = toKey(identifier);
            auto const& [result, value] = itemResult;
            if(!exists(identifier))
            {
                payload["errors"][key] = result.getErrorMessage();
            }
            else if(result.wasOk())
            {
                payload["results"][key][propertyName] = toJson(value);
            }
            else
            {
                payload["results"][key]["propertyErrors"][propertyName] = result.getErrorMessage();
            }
        }
    }

    // Gets an optional range argument (an object with a 'start' number field and an 'end' number field) or the default value if the argument is missing
    static std::optional<juce::Range<double>> getOptionalRangeArgument(nlohmann::json const& arguments, std::string const& name, std::optional<juce::Range<double>> const& defaultValue)
    {
        if(!arguments.contains(name))
        {
            return defaultValue;
        }
        auto const error = "The '" + name + "' field must be an object with a 'start' number field and an 'end' number field.";
        auto const& range = arguments.at(name);
        auto const start = getArgument<double>(range, "start", error);
        auto const end = getArgument<double>(range, "end", error);
        return juce::Range<double>{start, end};
    }

    // Gets an optional colour argument in the format #RRGGBBAA or throws an ExecutionError
    static std::optional<juce::Colour> getOptionalColourArgument(nlohmann::json const& arguments, std::string const& name)
    {
        if(!arguments.contains(name))
        {
            return {};
        }
        auto const error = "The '" + name + "' field must be a color string in the format #RRGGBBAA.";
        auto const colour = stringToColour(getArgument<std::string>(arguments, name, error));
        if(!colour.has_value())
        {
            throw ExecutionError(error);
        }
        return colour;
    }

    // Gets the graphics properties of an object
    static Operations::TrackGraphics getTrackGraphicsArgument(nlohmann::json const& arguments)
    {
        Operations::TrackGraphics graphics;
        graphics.colourMap = getOptionalEnumArgument<Track::ColourMap>(arguments, "colorMap");
        graphics.backgroundColour = getOptionalColourArgument(arguments, "colorBackground");
        graphics.foregroundColour = getOptionalColourArgument(arguments, "colorForeground");
        graphics.durationColour = getOptionalColourArgument(arguments, "colorDuration");
        graphics.textColour = getOptionalColourArgument(arguments, "colorText");
        graphics.shadowColour = getOptionalColourArgument(arguments, "colorShadow");
        graphics.fontName = getOptionalArgument<std::optional<juce::String>>(arguments, "fontName", {}, "The 'fontName' field must be a string.");
        graphics.fontStyle = getOptionalArgument<std::optional<juce::String>>(arguments, "fontStyle", {}, "The 'fontStyle' field must be a string.");
        graphics.fontSize = getOptionalArgument<std::optional<float>>(arguments, "fontSize", {}, "The 'fontSize' field must be a number.");
        graphics.lineWidth = getOptionalArgument<std::optional<float>>(arguments, "lineWidth", {}, "The 'lineWidth' field must be a number.");
        graphics.unit = getOptionalArgument<std::optional<juce::String>>(arguments, "unit", {}, "The 'unit' field must be a string.");
        graphics.labelLayoutPosition = getOptionalArgument<std::optional<float>>(arguments, "labelLayoutPosition", {}, "The 'labelLayoutPosition' field must be a number.");
        graphics.labelLayoutJustification = getOptionalEnumArgument<Track::LabelLayout::Justification>(arguments, "labelLayoutJustification");
        graphics.markerStyle = getOptionalEnumArgument<Track::MarkerStyle>(arguments, "markerStyle");
        return graphics;
    }

    // Gets the optional color gradient of the graphics properties or throws an ExecutionError
    static std::optional<Operations::TrackColourGradient> getOptionalColourGradientArgument(nlohmann::json const& arguments)
    {
        if(!arguments.contains("colorGradient"))
        {
            return {};
        }
        auto const& object = arguments.at("colorGradient");
        if(!object.is_object())
        {
            throw ExecutionError("The 'colorGradient' argument must be an object.");
        }
        Operations::TrackColourGradient gradient;
        auto const colourMap = getOptionalEnumArgument<Track::ColourMap>(object, "colorMap");
        if(!colourMap.has_value())
        {
            throw ExecutionError("The 'colorMap' field of the 'colorGradient' argument is required.");
        }
        gradient.colourMap = colourMap.value();

        using Member = std::optional<juce::Colour> Operations::TrackGraphics::*;
        static std::map<std::string, Member> const members{
            {"colorBackground", &Operations::TrackGraphics::backgroundColour},
            {"colorForeground", &Operations::TrackGraphics::foregroundColour},
            {"colorDuration", &Operations::TrackGraphics::durationColour},
            {"colorText", &Operations::TrackGraphics::textColour},
            {"colorShadow", &Operations::TrackGraphics::shadowColour}};
        auto const error = std::string("The 'properties' field of the 'colorGradient' argument is required and must be a non-empty array of the following strings: 'colorBackground', 'colorForeground', 'colorDuration', 'colorText', 'colorShadow'.");
        auto const names = getArgument<std::vector<std::string>>(object, "properties", error);
        if(names.empty())
        {
            throw ExecutionError(error);
        }
        for(auto const& name : names)
        {
            auto const it = members.find(name);
            if(it == members.cend())
            {
                throw ExecutionError(error);
            }
            if(std::find(gradient.properties.cbegin(), gradient.properties.cend(), it->second) == gradient.properties.cend())
            {
                gradient.properties.push_back(it->second);
            }
        }
        return gradient;
    }

    static auto constexpr trackAnchorName = "anchorTrackIdentifier";

    // Gets the placement and the anchor of an item, or the default ones if the item doesn't define a placement, or
    // throws an ExecutionError (the placement and the anchor are defined together, the anchor is required with the above
    // and below placements and is not allowed with the top and bottom placements)
    static std::optional<std::tuple<Operations::Placement, juce::String>> getOptionalPlacementArgument(nlohmann::json const& arguments, std::string const& anchorName, std::optional<std::tuple<Operations::Placement, juce::String>> const& defaultValue)
    {
        auto const placement = getOptionalEnumArgument<Operations::Placement>(arguments, "placement");
        auto const anchorIdentifier = getOptionalArgument<juce::String>(arguments, anchorName, {}, "The '" + anchorName + "' field must be a string.");
        if(!placement.has_value())
        {
            if(anchorIdentifier.isNotEmpty())
            {
                throw ExecutionError("The '" + anchorName + "' field must be defined with the 'placement' field ('above' or 'below') at the same level.");
            }
            return defaultValue;
        }
        auto const isRelative = placement.value() == Operations::Placement::above || placement.value() == Operations::Placement::below;
        if(isRelative && anchorIdentifier.isEmpty())
        {
            throw ExecutionError("The '" + anchorName + "' field is required with the 'above' and 'below' placements.");
        }
        if(!isRelative && anchorIdentifier.isNotEmpty())
        {
            throw ExecutionError("The '" + anchorName + "' field is only used with the 'above' and 'below' placements.");
        }
        return std::make_tuple(placement.value(), anchorIdentifier);
    }

    // Gets an optional number of linked tracks (a positive integer) or throws an ExecutionError
    static std::optional<size_t> getOptionalLinkedTrackCountArgument(nlohmann::json const& arguments, std::optional<size_t> const& defaultValue)
    {
        if(!arguments.contains("linkedTrackCount"))
        {
            return defaultValue;
        }
        auto const& value = arguments.at("linkedTrackCount");
        if(!value.is_number_unsigned() || value.get<size_t>() == 0_z)
        {
            throw ExecutionError("The 'linkedTrackCount' field must be a positive integer.");
        }
        return value.get<size_t>();
    }

    // Gets an optional plugin key (an object with an 'identifier' string field and a 'feature' string field) or throws an ExecutionError
    static std::optional<Plugin::Key> getOptionalPluginArgument(nlohmann::json const& arguments)
    {
        if(!arguments.contains("plugin"))
        {
            return {};
        }
        static auto constexpr error = "The 'plugin' field must be an object with an 'identifier' string field and a 'feature' string field.";
        auto const& plugin = arguments.at("plugin");
        Plugin::Key key;
        key.identifier = getArgument<std::string>(plugin, "identifier", error);
        key.feature = getArgument<std::string>(plugin, "feature", error);
        return key;
    }

    // Gets an optional time range (an array of two numbers [start, end] with start <= end) or throws an ExecutionError
    static std::optional<juce::Range<double>> getOptionalTimeRangeArgument(nlohmann::json const& arguments, std::string const& name)
    {
        if(!arguments.contains(name))
        {
            return {};
        }
        auto const error = "The '" + name + "' field must be an array of two numbers [startTime, endTime] with startTime <= endTime.";
        auto const values = getArgument<std::vector<double>>(arguments, name, error);
        if(values.size() != 2_z || values.at(0_z) > values.at(1_z))
        {
            throw ExecutionError(error);
        }
        return juce::Range<double>(values.at(0_z), values.at(1_z));
    }

    // Gets the selection of the results ('timeRanges', 'channels' and 'applyExtraThresholds' optional fields) or throws an ExecutionError
    static Operations::ResultsSelection getResultsSelectionArgument(nlohmann::json const& arguments)
    {
        static auto constexpr timeRangesError = "The 'timeRanges' field must be an array of time ranges [[startTime, endTime], ...] with startTime <= endTime.";
        static auto constexpr channelsError = "The 'channels' field must be an array of non-negative integers.";
        Operations::ResultsSelection selection;
        for(auto const& timeRange : getOptionalArgument<std::vector<nlohmann::json>>(arguments, "timeRanges", {}, timeRangesError))
        {
            auto const values = getArgument<std::vector<double>>(nlohmann::json{{"timeRange", timeRange}}, "timeRange", timeRangesError);
            if(values.size() != 2_z || values.at(0_z) > values.at(1_z))
            {
                throw ExecutionError(timeRangesError);
            }
            selection.timeRanges.emplace_back(values.at(0_z), values.at(1_z));
        }
        for(auto const& channel : getOptionalArgument<std::vector<nlohmann::json>>(arguments, "channels", {}, channelsError))
        {
            if(!channel.is_number_unsigned())
            {
                throw ExecutionError(channelsError);
            }
            selection.channels.insert(channel.get<size_t>());
        }
        selection.applyExtraThresholds = getOptionalArgument<bool>(arguments, "applyExtraThresholds", true, "The 'applyExtraThresholds' field must be a boolean.");
        return selection;
    }

    // Gets an optional one-character separator or throws an ExecutionError
    static char getOptionalSeparatorArgument(nlohmann::json const& arguments)
    {
        static auto constexpr error = "The 'separator' field must be a one-character string.";
        auto const separator = getOptionalArgument<std::string>(arguments, "separator", ",", error);
        if(separator.size() != 1_z)
        {
            throw ExecutionError(error);
        }
        return separator.front();
    }

    // Gets an optional non-negative number of seconds or throws an ExecutionError
    static double getOptionalWaitSecondsArgument(nlohmann::json const& arguments)
    {
        static auto constexpr error = "The 'waitSeconds' field must be a non-negative number.";
        auto const waitSeconds = getOptionalArgument<double>(arguments, "waitSeconds", 0.0, error);
        if(waitSeconds < 0.0)
        {
            throw ExecutionError(error);
        }
        return waitSeconds;
    }

    // Waits until the results of the tracks are available or the timeout is reached
    // The wait is skipped on the message thread because the results are updated on the message thread
    static void waitForTrackResults(Instance& instance, std::atomic<bool> const& shouldAbort, std::vector<juce::String> const& identifiers, double waitSeconds)
    {
        if(waitSeconds <= 0.0 || juce::MessageManager::existsAndIsCurrentThread())
        {
            return;
        }
        auto const deadline = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(waitSeconds));
        auto const isPending = [&]()
        {
            return callOnMessageThread(shouldAbort, [&]()
                                       {
                                           return Operations::hasTrackResultsPending(instance.getDocumentAccessor(), identifiers);
                                       });
        };
        while(isPending() && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Gets an optional array of objects with a 'key' string field and a 'value' number field as a map of keys to values
    static std::map<std::string, float> getParametersArgument(nlohmann::json const& arguments)
    {
        static auto constexpr error = "The 'parameters' field must be an array of objects with a 'key' string field and a 'value' number field.";
        std::map<std::string, float> values;
        for(auto const& parameter : getOptionalArgument<std::vector<nlohmann::json>>(arguments, "parameters", {}, error))
        {
            values.insert_or_assign(getArgument<std::string>(parameter, "key", error), getArgument<float>(parameter, "value", error));
        }
        return values;
    }

    // The responses of the operations on several items have the format {"results": {...}, "errors": {...}}
    // where the results and the errors are mapped by item (or {"results": [...]} where each item is a result or
    // an object with an "error" field when the order matters). The response is an error only if all the items failed.

    // Creates a response with the value or the error message of each item
    template <typename Key, typename T>
    static nlohmann::json createItemsResponse(std::map<Key, std::tuple<juce::Result, T>> const& results)
    {
        nlohmann::json payload;
        payload["results"] = nlohmann::json::object();
        payload["errors"] = nlohmann::json::object();
        for(auto const& [key, itemResult] : results)
        {
            auto const& [result, value] = itemResult;
            if(result.wasOk())
            {
                payload["results"][toKey(key)] = toJson(value);
            }
            else
            {
                payload["errors"][toKey(key)] = result.getErrorMessage();
            }
        }
        return createJsonResponse(payload, payload.at("results").empty() && !payload.at("errors").empty());
    }

    // Creates a response with the value or the error message of each item in the same order
    template <typename T>
    static nlohmann::json createItemsResponse(std::vector<std::tuple<juce::Result, T>> const& results)
    {
        nlohmann::json payload;
        payload["results"] = nlohmann::json::array();
        for(auto const& [result, value] : results)
        {
            payload["results"].push_back(result.wasOk() ? toJson(value) : nlohmann::json{{"error", result.getErrorMessage()}});
        }
        auto const isError = !results.empty() && std::all_of(results.cbegin(), results.cend(), [](auto const& item)
                                                             {
                                                                 return std::get<0>(item).failed();
                                                             });
        return createJsonResponse(payload, isError);
    }

    // Creates a response with the success message or the error message of each item
    template <typename Function>
    static nlohmann::json createItemsResponse(std::map<juce::String, juce::Result> const& results, Function&& getSuccessMessage)
    {
        nlohmann::json payload;
        payload["results"] = nlohmann::json::object();
        payload["errors"] = nlohmann::json::object();
        for(auto const& [identifier, result] : results)
        {
            if(result.wasOk())
            {
                payload["results"][toKey(identifier)] = getSuccessMessage(identifier);
            }
            else
            {
                payload["errors"][toKey(identifier)] = result.getErrorMessage();
            }
        }
        return createJsonResponse(payload, payload.at("results").empty() && !payload.at("errors").empty());
    }
} // namespace Application::Neuralyzer::Mcp

Application::Neuralyzer::Mcp::Handlers::Handlers(Instance& instance)
: mInstance(instance)
{
}

void Application::Neuralyzer::Mcp::Handlers::abort()
{
    mShouldAbort.store(true);
}

std::map<Plugin::Key, Plugin::Description> Application::Neuralyzer::Mcp::Handlers::getInstalledPlugins()
{
    return callOnMessageThread(mShouldAbort, [&]()
                               {
                                   return std::get<0_z>(mInstance.getPluginListScanner().getPlugins(48000.0, false));
                               });
}

std::vector<Plugin::WebReference> Application::Neuralyzer::Mcp::Handlers::getWebPlugins()
{
    return callOnMessageThread(mShouldAbort, [&]()
                               {
                                   auto const installedPlugins = std::get<0_z>(mInstance.getPluginListScanner().getPlugins(48000.0, false));
                                   std::vector<Plugin::WebReference> webPlugins;
                                   for(auto const& reference : mInstance.getPluginListAccessor().getAttr<PluginList::AttrType::webReferences>())
                                   {
                                       auto const isInstalled = std::any_of(installedPlugins.cbegin(), installedPlugins.cend(), [&](auto const& pair)
                                                                            {
                                                                                return juce::String(pair.first.identifier).startsWith(reference.identifier + ":");
                                                                            });
                                       if(!isInstalled && reference.isCompatible)
                                       {
                                           webPlugins.push_back(reference);
                                       }
                                   }
                                   return webPlugins;
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getDocumentState(nlohmann::json const& arguments)
{
    // All the properties are returned if the properties are not specified
    auto const properties = arguments.contains("properties") ? getPropertiesArgument<DocumentProperty>(arguments) : std::vector<DocumentProperty>(magic_enum::enum_values<DocumentProperty>().cbegin(), magic_enum::enum_values<DocumentProperty>().cend());
    auto const payload = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 auto const& documentAcsr = mInstance.getDocumentAccessor();
                                                 auto json = nlohmann::json::object();
                                                 for(auto const property : properties)
                                                 {
                                                     auto const propertyName = std::string(magic_enum::enum_name(property));
                                                     switch(property)
                                                     {
                                                         case DocumentProperty::groups:
                                                             json[propertyName] = Operations::getDocumentState(documentAcsr);
                                                             break;
                                                         case DocumentProperty::timeZoom:
                                                             json[propertyName] = Operations::getDocumentTimeZoom(documentAcsr);
                                                             break;
                                                         case DocumentProperty::timeSelection:
                                                         {
                                                             auto const selection = Operations::getTimeSelection(documentAcsr);
                                                             json[propertyName] = selection;
                                                             json[propertyName]["isEmpty"] = selection.isEmpty();
                                                             break;
                                                         }
                                                         case DocumentProperty::selectedItems:
                                                             json[propertyName] = Operations::getSelectedItems(documentAcsr);
                                                             break;
                                                         case DocumentProperty::audioFileLayout:
                                                             json[propertyName] = Operations::getDocumentAudioFileLayout(documentAcsr);
                                                             break;
                                                     }
                                                 }
                                                 return json;
                                             });
    return createJsonResponse(payload, false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setDocumentAudioFileLayout(nlohmann::json const& arguments)
{
    std::vector<AudioFileLayout> layouts;
    for(auto const& item : getArgument<std::vector<nlohmann::json>>(arguments, "layout", "The 'layout' field is required and must be an array."))
    {
        auto const filePath = getArgument<std::string>(item, "file", "Each layout item must be an object with a 'file' string field.");
        if(!item.contains("channel") || !item.at("channel").is_number_integer())
        {
            throw ExecutionError("Each layout item must have a 'channel' integer field.");
        }
        auto const channel = getArgument<int>(item, "channel", "Each layout item must have a 'channel' integer field.");
        layouts.push_back(AudioFileLayout(juce::File(filePath), channel));
    }
    callOnMessageThread(mShouldAbort, [&]()
                        {
                            Operations::setDocumentAudioFileLayout(mInstance.getDocumentAccessor(), layouts);
                        });
    return createTextResponse("Audio file layout updated successfully", false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getAudioFileInfo(nlohmann::json const& arguments)
{
    auto const files = getArgument<std::vector<juce::File>>(arguments, "files", "The 'files' argument is required and must be an array of strings.");
    // The audio format manager is not modified once the application is initialized and can be used on this thread
    return createItemsResponse(Operations::getAudioFileInfo(mInstance.getAudioFormatManager(), files));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setDocumentTimeZoom(nlohmann::json const& arguments)
{
    auto const start = getArgument<double>(arguments, "start", "The 'start' field is required and must be a number.");
    auto const end = getArgument<double>(arguments, "end", "The 'end' field is required and must be a number.");
    auto const [result, ranges] = callOnMessageThread(mShouldAbort, [&]()
                                                      {
                                                          return Operations::setDocumentTimeZoom(mInstance.getDocumentAccessor(), {start, end});
                                                      });
    if(result.failed())
    {
        return createTextResponse(result.getErrorMessage().toStdString(), true);
    }
    return createJsonResponse(ranges, false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getGroupProperties(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const properties = getPropertiesArgument<GroupProperty>(arguments);
    auto const payload = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 auto const& documentAcsr = mInstance.getDocumentAccessor();
                                                 nlohmann::json json;
                                                 json["results"] = nlohmann::json::object();
                                                 json["errors"] = nlohmann::json::object();
                                                 auto const addValues = [&](GroupProperty property, auto const& values)
                                                 {
                                                     addPropertyValues(json, property, values, [&](juce::String const& identifier)
                                                                       {
                                                                           return Document::Tools::hasGroupAcsr(documentAcsr, identifier);
                                                                       });
                                                 };
                                                 for(auto const property : properties)
                                                 {
                                                     switch(property)
                                                     {
                                                         case GroupProperty::name:
                                                             addValues(property, Operations::getGroupNames(documentAcsr, identifiers));
                                                             break;
                                                         case GroupProperty::layout:
                                                             addValues(property, Operations::getGroupLayouts(documentAcsr, identifiers));
                                                             break;
                                                         case GroupProperty::verticalZoom:
                                                             addValues(property, Operations::getGroupVerticalZoom(documentAcsr, identifiers));
                                                             break;
                                                     }
                                                 }
                                                 return json;
                                             });
    return createJsonResponse(payload, payload.at("results").empty() && !payload.at("errors").empty());
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setGroupProperties(nlohmann::json const& arguments)
{
    // The top-level vertical zoom is a default value applied to all the groups, the properties of a group override it
    auto const defaultVerticalZoom = getOptionalRangeArgument(arguments, "verticalZoom", std::nullopt);
    std::map<juce::String, Operations::GroupPropertiesModification> modifications;
    for(auto const& [identifier, group] : getItemsArgument(arguments, "groups", "The 'groups' argument is required and must be an array of objects."))
    {
        Operations::GroupPropertiesModification modification;
        modification.name = getOptionalArgument<std::optional<juce::String>>(group, "name", std::nullopt, "The 'name' field must be a string.");
        modification.layout = getOptionalArgument<std::optional<std::vector<juce::String>>>(group, "layout", std::nullopt, "The 'layout' field must be an array of strings.");
        modification.verticalZoom = getOptionalRangeArgument(group, "verticalZoom", defaultVerticalZoom);
        if(!modification.name.has_value() && !modification.layout.has_value() && !modification.verticalZoom.has_value())
        {
            throw ExecutionError(juce::String(R"(No property to modify for the group "GROUPID".)").replace("GROUPID", identifier).toStdString());
        }
        modifications.insert_or_assign(identifier, modification);
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setGroupProperties(mInstance.getDocumentDirector(), modifications);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::createGroups(nlohmann::json const& arguments)
{
    static auto constexpr groupsError = "The 'groups' argument is required and must be an array of objects.";
    static auto constexpr anchorName = "anchorGroupIdentifier";
    auto const defaultPlacement = getOptionalPlacementArgument(arguments, anchorName, std::make_tuple(Operations::Placement::bottom, juce::String{}));
    std::vector<Operations::GroupCreation> groups;
    for(auto const& group : getArgument<std::vector<nlohmann::json>>(arguments, "groups", groupsError))
    {
        if(!group.is_object())
        {
            throw ExecutionError(groupsError);
        }
        Operations::GroupCreation creation;
        creation.name = getOptionalArgument<juce::String>(group, "name", {}, "The 'name' field must be a string.");
        std::tie(creation.placement, creation.anchorIdentifier) = getOptionalPlacementArgument(group, anchorName, defaultPlacement).value();
        groups.push_back(std::move(creation));
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::createGroups(mInstance.getDocumentDirector(), groups);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::removeGroups(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::removeGroups(mInstance.getDocumentDirector(), identifiers);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The group has been removed.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackProperties(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const properties = getPropertiesArgument<TrackProperty>(arguments);
    auto const payload = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 auto& documentDir = mInstance.getDocumentDirector();
                                                 auto const& documentAcsr = mInstance.getDocumentAccessor();
                                                 nlohmann::json json;
                                                 json["results"] = nlohmann::json::object();
                                                 json["errors"] = nlohmann::json::object();
                                                 auto const addValues = [&](TrackProperty property, auto const& values)
                                                 {
                                                     addPropertyValues(json, property, values, [&](juce::String const& identifier)
                                                                       {
                                                                           return Document::Tools::hasTrackAcsr(documentAcsr, identifier);
                                                                       });
                                                 };
                                                 for(auto const property : properties)
                                                 {
                                                     switch(property)
                                                     {
                                                         case TrackProperty::name:
                                                             addValues(property, Operations::getTrackNames(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::type:
                                                             addValues(property, Operations::getTrackTypes(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::description:
                                                             addValues(property, Operations::getTrackDescriptions(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::parameters:
                                                             addValues(property, Operations::getTrackParameters(documentDir, identifiers));
                                                             break;
                                                         case TrackProperty::graphics:
                                                             addValues(property, Operations::getTrackGraphics(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::inputs:
                                                             addValues(property, Operations::getTrackInputs(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::visibleInGroup:
                                                             addValues(property, Operations::getTrackIsVisibleInGroup(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::extraThresholds:
                                                             addValues(property, Operations::getTrackExtraThresholds(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::verticalZoom:
                                                             addValues(property, Operations::getTrackVerticalZoom(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::valueRange:
                                                             addValues(property, Operations::getTrackValueRange(documentAcsr, identifiers));
                                                             break;
                                                         case TrackProperty::linkedTracks:
                                                             addValues(property, Operations::getLinkedTracks(documentAcsr, identifiers));
                                                             break;
                                                     }
                                                 }
                                                 return json;
                                             });
    return createJsonResponse(payload, payload.at("results").empty() && !payload.at("errors").empty());
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackProperties(nlohmann::json const& arguments)
{
    static auto constexpr thresholdsError = "The 'extraThresholds' field must be an array of numbers or null values.";
    // The top-level properties are default values applied to all the tracks, the properties of a track override them
    auto const getModification = [&](nlohmann::json const& object, Operations::TrackPropertiesModification const& defaultValues)
    {
        Operations::TrackPropertiesModification modification;
        modification.name = getOptionalArgument<std::optional<juce::String>>(object, "name", defaultValues.name, "The 'name' field must be a string.");
        modification.visibleInGroup = getOptionalArgument<std::optional<bool>>(object, "visibleInGroup", defaultValues.visibleInGroup, "The 'visibleInGroup' field must be a boolean.");
        // An empty array is a valid value (that removes the thresholds)
        modification.extraThresholds = getOptionalArgument<std::optional<std::vector<std::optional<float>>>>(object, "extraThresholds", defaultValues.extraThresholds, thresholdsError);
        modification.verticalZoom = getOptionalRangeArgument(object, "verticalZoom", defaultValues.verticalZoom);
        modification.valueRange = getOptionalRangeArgument(object, "valueRange", defaultValues.valueRange);
        return modification;
    };
    auto const defaultModification = getModification(arguments, {});
    std::map<juce::String, Operations::TrackPropertiesModification> modifications;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        auto const modification = getModification(track, defaultModification);
        if(!modification.name.has_value() && !modification.visibleInGroup.has_value() && !modification.extraThresholds.has_value() && !modification.verticalZoom.has_value() && !modification.valueRange.has_value())
        {
            throw ExecutionError(juce::String(R"(No property to modify for the track "TRACKID".)").replace("TRACKID", identifier).toStdString());
        }
        modifications.insert_or_assign(identifier, modification);
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setTrackProperties(mInstance.getDocumentDirector(), modifications);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackCompatibleInputs(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    std::map<juce::String, std::set<juce::String>> inputs;
    for(auto const& track : getArgument<std::vector<nlohmann::json>>(arguments, "tracks", tracksError))
    {
        if(!track.is_object())
        {
            throw ExecutionError(tracksError);
        }
        auto const identifier = getArgument<juce::String>(track, "receiverTrackIdentifier", "The 'receiverTrackIdentifier' field is required and must be a string.");
        auto const inputIdentifier = getArgument<juce::String>(track, "receiverInputIdentifier", "The 'receiverInputIdentifier' field is required and must be a string.");
        inputs[identifier].insert(inputIdentifier);
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackCompatibleInputs(mInstance.getDocumentDirector(), inputs);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackGraphics(nlohmann::json const& arguments)
{
    auto const defaultGraphics = getTrackGraphicsArgument(arguments);
    auto const gradient = getOptionalColourGradientArgument(arguments);
    // The order of the tracks is preserved for the gradient
    std::vector<std::pair<juce::String, Operations::TrackGraphics>> graphics;
    for(auto const& [identifier, track] : getOrderedItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        graphics.emplace_back(identifier, getTrackGraphicsArgument(track));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackGraphics(mInstance.getDocumentDirector(), defaultGraphics, graphics, gradient);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The graphics properties have been modified.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackParameters(nlohmann::json const& arguments)
{
    auto const defaultValues = getParametersArgument(arguments);
    std::map<juce::String, std::map<std::string, float>> values;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        auto trackValues = getParametersArgument(track);
        if(trackValues.empty() && defaultValues.empty())
        {
            throw ExecutionError("The 'parameters' field is required either at the top-level or per track.");
        }
        values.insert_or_assign(identifier, std::move(trackValues));
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setTrackParameters(mInstance.getDocumentDirector(), defaultValues, values);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setLinkedTrackCount(nlohmann::json const& arguments)
{
    auto const defaultCount = getOptionalLinkedTrackCountArgument(arguments, std::nullopt);
    std::map<juce::String, size_t> counts;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        auto const count = getOptionalLinkedTrackCountArgument(track, defaultCount);
        if(!count.has_value())
        {
            throw ExecutionError("The 'linkedTrackCount' field is required either at the top-level or per track.");
        }
        counts.insert_or_assign(identifier, count.value());
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setLinkedTrackCount(mInstance.getDocumentDirector(), counts);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackInputs(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    static auto constexpr inputError = "The 'receiverInputIdentifier' field must be a string.";
    static auto constexpr senderError = "The 'senderTrackIdentifier' field must be a string.";
    auto const defaultInputIdentifier = getOptionalArgument<std::optional<juce::String>>(arguments, "receiverInputIdentifier", {}, inputError);
    auto const defaultSenderIdentifier = getOptionalArgument<std::optional<juce::String>>(arguments, "senderTrackIdentifier", {}, senderError);
    std::map<juce::String, std::map<juce::String, juce::String>> inputs;
    for(auto const& track : getArgument<std::vector<nlohmann::json>>(arguments, "tracks", tracksError))
    {
        if(!track.is_object())
        {
            throw ExecutionError(tracksError);
        }
        auto const identifier = getArgument<juce::String>(track, "receiverTrackIdentifier", "The 'receiverTrackIdentifier' field is required and must be a string.");
        auto const inputIdentifier = getOptionalArgument<std::optional<juce::String>>(track, "receiverInputIdentifier", defaultInputIdentifier, inputError);
        if(!inputIdentifier.has_value())
        {
            throw ExecutionError("The 'receiverInputIdentifier' field is required either at the top-level or per track.");
        }
        auto const senderIdentifier = getOptionalArgument<std::optional<juce::String>>(track, "senderTrackIdentifier", defaultSenderIdentifier, senderError);
        if(!senderIdentifier.has_value())
        {
            throw ExecutionError("The 'senderTrackIdentifier' field is required either at the top-level or per track.");
        }
        inputs[identifier].insert_or_assign(inputIdentifier.value(), senderIdentifier.value());
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackInputs(mInstance.getDocumentDirector(), inputs);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The inputs have been modified.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::createTracksWithPlugins(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    static auto constexpr groupError = "The 'groupIdentifier' field must be a string.";
    auto const defaultPlugin = getOptionalPluginArgument(arguments);
    auto const defaultGroup = getOptionalArgument<juce::String>(arguments, "groupIdentifier", {}, groupError);
    auto const defaultParameters = getParametersArgument(arguments);
    auto const defaultLinkedTrackCount = getOptionalLinkedTrackCountArgument(arguments, std::nullopt);
    std::vector<Operations::PluginTrackCreation> tracks;
    for(auto const& track : getArgument<std::vector<nlohmann::json>>(arguments, "tracks", tracksError))
    {
        if(!track.is_object())
        {
            throw ExecutionError(tracksError);
        }
        Operations::PluginTrackCreation creation;
        auto const plugin = getOptionalPluginArgument(track);
        if(!plugin.has_value() && !defaultPlugin.has_value())
        {
            throw ExecutionError("The 'plugin' field is required either at the top-level or per track.");
        }
        creation.key = plugin.value_or(defaultPlugin.value_or(Plugin::Key{}));
        creation.groupIdentifier = getOptionalArgument<juce::String>(track, "groupIdentifier", defaultGroup, groupError);
        creation.name = getOptionalArgument<std::optional<juce::String>>(track, "name", {}, "The 'name' field must be a string.");
        creation.parameters = getParametersArgument(track);
        creation.linkedTrackCount = getOptionalLinkedTrackCountArgument(track, defaultLinkedTrackCount);
        tracks.push_back(std::move(creation));
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       auto const& globalGraphicPreset = mInstance.getApplicationAccessor().getAttr<Application::AttrType::globalGraphicPreset>();
                                                       return Operations::createTracksWithPlugins(mInstance.getDocumentDirector(), mInstance.getTrackPresetListAccessor(), globalGraphicPreset, defaultParameters, tracks);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::createTracksWithFiles(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    static auto constexpr groupError = "The 'groupIdentifier' field must be a string.";
    auto const defaultGroup = getOptionalArgument<juce::String>(arguments, "groupIdentifier", {}, groupError);
    std::vector<Operations::FileTrackCreation> tracks;
    for(auto const& track : getArgument<std::vector<nlohmann::json>>(arguments, "tracks", tracksError))
    {
        if(!track.is_object())
        {
            throw ExecutionError(tracksError);
        }
        Operations::FileTrackCreation creation;
        creation.file = getArgument<juce::File>(track, "file", "The 'file' field is required and must be a string.");
        creation.groupIdentifier = getOptionalArgument<juce::String>(track, "groupIdentifier", defaultGroup, groupError);
        tracks.push_back(std::move(creation));
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       auto const& globalGraphicPreset = mInstance.getApplicationAccessor().getAttr<Application::AttrType::globalGraphicPreset>();
                                                       return Operations::createTracksWithFiles(mInstance.getDocumentDirector(), globalGraphicPreset, tracks);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::removeTracks(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::removeTracks(mInstance.getDocumentDirector(), identifiers);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The track has been removed.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::moveTracks(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    static auto constexpr groupError = "The 'groupIdentifier' field must be a string.";
    auto const defaultGroup = getOptionalArgument<juce::String>(arguments, "groupIdentifier", {}, groupError);
    auto const defaultPlacement = getOptionalPlacementArgument(arguments, trackAnchorName, std::nullopt);
    std::vector<Operations::TrackMove> moves;
    for(auto const& track : getArgument<std::vector<nlohmann::json>>(arguments, "tracks", tracksError))
    {
        if(!track.is_object())
        {
            throw ExecutionError(tracksError);
        }
        Operations::TrackMove move;
        move.identifier = getArgument<juce::String>(track, "identifier", "The 'identifier' field is required and must be a string.");
        move.groupIdentifier = getOptionalArgument<juce::String>(track, "groupIdentifier", defaultGroup, groupError);
        auto const placement = getOptionalPlacementArgument(track, trackAnchorName, defaultPlacement);
        if(!placement.has_value())
        {
            throw ExecutionError("The 'placement' field is required either at the top-level or per track.");
        }
        std::tie(move.placement, move.anchorIdentifier) = placement.value();
        moves.push_back(std::move(move));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::moveTracks(mInstance.getDocumentDirector(), moves);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The track has been moved.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackResultsRaw(nlohmann::json const& arguments)
{
    static auto constexpr maxResultsWithoutLargeOutput = 20_z;
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const selection = getResultsSelectionArgument(arguments);
    auto const headerType = getOptionalEnumArgument<Track::Exporter::CsvHeaderType>(arguments, "header").value_or(Track::Exporter::CsvHeaderType::generic);
    auto const separator = getOptionalSeparatorArgument(arguments);
    auto const allowLargeOutput = getOptionalArgument<bool>(arguments, "allowLargeOutput", false, "The 'allowLargeOutput' field must be a boolean.");
    waitForTrackResults(mInstance, mShouldAbort, identifiers, getOptionalWaitSecondsArgument(arguments));
    if(!allowLargeOutput)
    {
        // The results are counted on this thread
        auto const snapshots = callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackResultsSnapshots(mInstance.getDocumentAccessor(), identifiers);
                                                   });
        auto const total = std::accumulate(snapshots.cbegin(), snapshots.cend(), 0_z, [&](size_t value, auto const& item)
                                           {
                                               auto const& [result, snapshot] = item.second;
                                               return result.wasOk() ? value + std::get<1_z>(Operations::countResults(snapshot, selection)) : value;
                                           });
        if(total > maxResultsWithoutLargeOutput)
        {
            auto const message = juce::String("The request would export COUNT results (more than MAX). Use 'get_track_results_summary' first, or set 'allowLargeOutput' to true only when strictly necessary.").replace("COUNT", juce::String(total)).replace("MAX", juce::String(maxResultsWithoutLargeOutput));
            return createTextResponse(message.toStdString(), true);
        }
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackResultsRaw(mInstance.getDocumentAccessor(), identifiers, selection, headerType, separator);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackResultsSummary(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const selection = getResultsSelectionArgument(arguments);
    waitForTrackResults(mInstance, mShouldAbort, identifiers, getOptionalWaitSecondsArgument(arguments));
    // The results are summarized on this thread
    auto const snapshots = callOnMessageThread(mShouldAbort, [&]()
                                               {
                                                   return Operations::getTrackResultsSnapshots(mInstance.getDocumentAccessor(), identifiers);
                                               });
    std::map<juce::String, std::tuple<juce::Result, Operations::ResultsSummary>> summaries;
    for(auto const& [identifier, snapshotResult] : snapshots)
    {
        auto const& [result, snapshot] = snapshotResult;
        summaries.insert_or_assign(identifier, result.wasOk() ? Operations::summarizeResults(snapshot, selection) : std::make_tuple(result, Operations::ResultsSummary{}));
    }
    return createItemsResponse(summaries);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackResultsRaw(nlohmann::json const& arguments)
{
    auto const separator = getOptionalSeparatorArgument(arguments);
    std::map<juce::String, Operations::ResultsModification> modifications;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        Operations::ResultsModification modification;
        modification.results = getArgument<std::string>(track, "results", "The 'results' field is required and must be a CSV string.");
        modification.timeRange = getOptionalTimeRangeArgument(track, "timeRange");
        modifications.insert_or_assign(identifier, std::move(modification));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackResultsRaw(mInstance.getDocumentDirector(), modifications, separator);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The results have been modified.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::eraseTrackResults(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    auto const selection = getResultsSelectionArgument(arguments);
    if(selection.timeRanges.empty())
    {
        throw ExecutionError("The 'timeRanges' argument is required and must be a non-empty array of time ranges [[startTime, endTime], ...] with startTime <= endTime.");
    }
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::eraseTrackResults(mInstance.getDocumentDirector(), identifiers, selection.timeRanges, selection.channels);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::saveTrackResults(nlohmann::json const& arguments)
{
    using Options = Document::Exporter::Options;
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    if(identifiers.empty())
    {
        throw ExecutionError("There is no track or group to export.");
    }
    auto const format = getOptionalEnumArgument<Options::Format>(arguments, "format");
    if(!format.has_value())
    {
        throw ExecutionError("The 'format' argument is required.");
    }
    Options options;
    options.format = format.value();
    options.useAutoSize = getOptionalArgument<bool>(arguments, "useAutoSize", options.useAutoSize, "The 'useAutoSize' field must be a boolean.");
    options.imageWidth = getOptionalArgument<int>(arguments, "imageWidth", options.imageWidth, "The 'imageWidth' field must be a number.");
    options.imageHeight = getOptionalArgument<int>(arguments, "imageHeight", options.imageHeight, "The 'imageHeight' field must be a number.");
    options.imagePpi = getOptionalArgument<int>(arguments, "imagePpi", options.imagePpi, "The 'imagePpi' field must be a number.");
    options.csvHeaderType = getOptionalEnumArgument<Options::CsvHeaderType>(arguments, "csvHeaderType").value_or(options.csvHeaderType);
    options.applyExtraThresholds = getOptionalArgument<bool>(arguments, "applyExtraThresholds", options.applyExtraThresholds, "The 'applyExtraThresholds' field must be a boolean.");
    options.reaperType = getOptionalEnumArgument<Options::ReaperType>(arguments, "reaperType").value_or(options.reaperType);
    options.columnSeparator = getOptionalEnumArgument<Options::ColumnSeparator>(arguments, "columnSeparator").value_or(options.columnSeparator);
    options.labSeparator = getOptionalEnumArgument<Options::ColumnSeparator>(arguments, "labSeparator").value_or(options.labSeparator);
    options.disableLabelEscaping = getOptionalArgument<bool>(arguments, "disableLabelEscaping", options.disableLabelEscaping, "The 'disableLabelEscaping' field must be a boolean.");
    options.includeDescription = getOptionalArgument<bool>(arguments, "includeDescription", options.includeDescription, "The 'includeDescription' field must be a boolean.");
    options.sdifFrameSignature = getOptionalArgument<juce::String>(arguments, "sdifFrameSignature", options.sdifFrameSignature, "The 'sdifFrameSignature' field must be a string.");
    options.sdifMatrixSignature = getOptionalArgument<juce::String>(arguments, "sdifMatrixSignature", options.sdifMatrixSignature, "The 'sdifMatrixSignature' field must be a string.");
    options.sdifColumnName = getOptionalArgument<juce::String>(arguments, "sdifColumnName", options.sdifColumnName, "The 'sdifColumnName' field must be a string.");
    if(arguments.contains("outsideGridJustification"))
    {
        static auto constexpr error = "The 'outsideGridJustification' field must be an array containing 'none' or any of these strings: 'left', 'right', 'top', 'bottom'.";
        static std::map<std::string, int> const flags{{"left", Zoom::Grid::Justification::left}, {"right", Zoom::Grid::Justification::right}, {"top", Zoom::Grid::Justification::top}, {"bottom", Zoom::Grid::Justification::bottom}};
        auto const names = getArgument<std::vector<std::string>>(arguments, "outsideGridJustification", error);
        auto justification = 0;
        for(auto const& name : names)
        {
            if(name == "none" && names.size() == 1_z)
            {
                continue;
            }
            auto const it = flags.find(name);
            if(it == flags.cend())
            {
                throw ExecutionError(error);
            }
            justification |= it->second;
        }
        options.outsideGridJustification = Zoom::Grid::Justification(justification);
    }

    if(!options.isValid())
    {
        throw ExecutionError("The SDIF frame and matrix signatures must be 4-character signatures without '?'.");
    }

    // The groups are replaced by their tracks with the text and binary formats, and the items that can't be exported are reported as errors
    auto const items = callOnMessageThread(mShouldAbort, [&]()
                                           {
                                               return Operations::getExportItems(mInstance.getDocumentAccessor(), identifiers, options);
                                           });
    std::vector<juce::String> exportIdentifiers;
    std::map<juce::String, std::tuple<juce::Result, std::vector<juce::File>>> results;
    for(auto const& [identifier, result] : items)
    {
        if(result.wasOk())
        {
            exportIdentifiers.push_back(identifier);
        }
        else
        {
            results.insert_or_assign(identifier, std::make_tuple(result, std::vector<juce::File>{}));
        }
    }

    nlohmann::json payload;
    if(!exportIdentifiers.empty())
    {
        auto const directory = getTempExportDirectory().getChildFile(juce::Uuid().toString());
        auto const directoryResult = directory.createDirectory();
        if(directoryResult.failed())
        {
            throw ExecutionError(directoryResult.getErrorMessage().toStdString());
        }
        payload["directory"] = directory;
        // As for the export of the application, the export is performed on this thread
        results.merge(Operations::saveTrackResults(mInstance.getDocumentAccessor(), exportIdentifiers, options, directory, mShouldAbort));
    }
    payload["results"] = nlohmann::json::object();
    payload["errors"] = nlohmann::json::object();
    for(auto const& [identifier, itemResult] : results)
    {
        auto const& [result, files] = itemResult;
        if(result.wasOk())
        {
            payload["results"][toKey(identifier)] = files;
        }
        else
        {
            payload["errors"][toKey(identifier)] = result.getErrorMessage();
        }
    }
    return createJsonResponse(payload, payload.at("results").empty());
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getVampPluginDescriptions(nlohmann::json const& arguments)
{
    static auto constexpr keysError = "The 'keys' argument is required and must be a non-empty array of objects containing an 'identifier' string field and a 'feature' string field.";
    std::vector<Plugin::Key> keys;
    for(auto const& keyJson : getArgument<std::vector<nlohmann::json>>(arguments, "keys", keysError))
    {
        Plugin::Key key;
        key.identifier = getArgument<std::string>(keyJson, "identifier", keysError);
        key.feature = getArgument<std::string>(keyJson, "feature", keysError);
        keys.push_back(std::move(key));
    }
    if(keys.empty())
    {
        throw ExecutionError(keysError);
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::getVampPluginDescriptions(mInstance.getPluginListScanner(), keys);
                                             });
    nlohmann::json payload;
    payload["results"] = nlohmann::json::array();
    for(auto index = 0_z; index < keys.size(); ++index)
    {
        auto const& [result, description] = results.at(index);
        nlohmann::json item;
        item["key"] = keys.at(index);
        if(result.wasOk())
        {
            item["description"] = toJson(description);
        }
        else
        {
            item["error"] = result.getErrorMessage();
        }
        payload["results"].push_back(std::move(item));
    }
    auto const isError = std::all_of(results.cbegin(), results.cend(), [](auto const& item)
                                     {
                                         return std::get<0>(item).failed();
                                     });
    return createJsonResponse(payload, isError);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::searchDocs(nlohmann::json const& arguments, DynamicToolMethods::search_docs_fn_t const& searchDocsFn)
{
    static auto constexpr maxNumDocumentsError = "The 'maxNumDocuments' argument must be a positive integer.";
    auto const query = getArgument<std::string>(arguments, "query", "The 'query' argument is required and must be a string.");
    if(arguments.contains("maxNumDocuments") && !arguments.at("maxNumDocuments").is_number_unsigned())
    {
        throw ExecutionError(maxNumDocumentsError);
    }
    auto const maxNumDocuments = getOptionalArgument<size_t>(arguments, "maxNumDocuments", 10_z, maxNumDocumentsError);
    if(maxNumDocuments == 0_z)
    {
        throw ExecutionError(maxNumDocumentsError);
    }
    return searchDocsFn(query, maxNumDocuments);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::loadDocs(nlohmann::json const& arguments, DynamicToolMethods::load_docs_fn_t const& loadDocsFn)
{
    static auto constexpr idsError = "The 'ids' argument is required and must be a non-empty array of strings.";
    auto const ids = getArgument<std::vector<std::string>>(arguments, "ids", idsError);
    if(ids.empty())
    {
        throw ExecutionError(idsError);
    }
    return loadDocsFn(ids);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::readFiles(nlohmann::json const& arguments, DynamicToolMethods::read_files_fn_t const& readFilesFn)
{
    static auto constexpr filesError = "The 'files' argument is required and must be a non-empty array of strings.";
    auto const files = getArgument<std::vector<std::string>>(arguments, "files", filesError);
    if(files.empty())
    {
        throw ExecutionError(filesError);
    }
    return readFilesFn(files);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::elicit(nlohmann::json const& arguments, DynamicToolMethods::elicit_fn_t const& elicitFn)
{
    static auto constexpr messageError = "The 'message' argument is required and must be a non-empty string.";
    auto const message = getArgument<std::string>(arguments, "message", messageError);
    if(juce::String::fromUTF8(message.c_str()).trim().isEmpty())
    {
        throw ExecutionError(messageError);
    }
    auto const options = getOptionalArgument<std::vector<std::string>>(arguments, "options", {}, "The 'options' argument must be an array of strings.");
    auto const answer = elicitFn(message, options);
    if(!answer.has_value())
    {
        throw ExecutionError("The question has been cancelled.");
    }
    return createTextResponse(answer.value(), false);
}

ANALYSE_FILE_END
