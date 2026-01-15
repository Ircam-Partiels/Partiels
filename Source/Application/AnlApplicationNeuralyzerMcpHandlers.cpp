#include "AnlApplicationNeuralyzerMcpHandlers.h"
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

    // Gets an array of objects with an 'identifier' string field as a map of identifiers to objects
    static std::map<juce::String, nlohmann::json> getItemsArgument(nlohmann::json const& arguments, std::string const& name, std::string const& error)
    {
        std::map<juce::String, nlohmann::json> items;
        for(auto const& item : getArgument<std::vector<nlohmann::json>>(arguments, name, error))
        {
            if(!item.is_object())
            {
                throw ExecutionError(error);
            }
            auto const identifier = getArgument<std::string>(item, "identifier", "The 'identifier' field is required and must be a string.");
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
        return json;
    }

    // Gets an array of objects with an 'identifier' string field, a 'start' number field and an 'end' number field as a map of identifiers to ranges
    static std::map<juce::String, juce::Range<double>> getRangesArgument(nlohmann::json const& arguments, std::string const& name, std::string const& error)
    {
        std::map<juce::String, juce::Range<double>> ranges;
        for(auto const& [identifier, item] : getItemsArgument(arguments, name, error))
        {
            auto const start = getArgument<double>(item, "start", "The 'start' field is required and must be a number.");
            auto const end = getArgument<double>(item, "end", "The 'end' field is required and must be a number.");
            ranges.insert_or_assign(identifier, juce::Range<double>{start, end});
        }
        return ranges;
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

    // Gets an optional position (a non-negative integer) or throws an ExecutionError
    static std::optional<size_t> getOptionalPositionArgument(nlohmann::json const& arguments)
    {
        if(!arguments.contains("position"))
        {
            return {};
        }
        if(!arguments.at("position").is_number_unsigned())
        {
            throw ExecutionError("The 'position' field must be a non-negative integer.");
        }
        return arguments.at("position").get<size_t>();
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getDocumentState([[maybe_unused]] nlohmann::json const& arguments)
{
    auto const groups = callOnMessageThread(mShouldAbort, [&]()
                                            {
                                                return Operations::getDocumentState(mInstance.getDocumentAccessor());
                                            });
    nlohmann::json payload;
    payload["groups"] = groups;
    return createJsonResponse(payload, false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getDocumentAudioFileLayout([[maybe_unused]] nlohmann::json const& arguments)
{
    auto const layouts = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::getDocumentAudioFileLayout(mInstance.getDocumentAccessor());
                                             });
    nlohmann::json payload;
    payload["layout"] = layouts;
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getDocumentTimeZoom([[maybe_unused]] nlohmann::json const& arguments)
{
    auto const ranges = callOnMessageThread(mShouldAbort, [&]()
                                            {
                                                return Operations::getDocumentTimeZoom(mInstance.getDocumentAccessor());
                                            });
    return createJsonResponse(ranges, false);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTimeSelection([[maybe_unused]] nlohmann::json const& arguments)
{
    auto const selection = callOnMessageThread(mShouldAbort, [&]()
                                               {
                                                   return Operations::getTimeSelection(mInstance.getDocumentAccessor());
                                               });
    nlohmann::json payload = selection;
    payload["isEmpty"] = selection.isEmpty();
    return createJsonResponse(payload, false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getSelectedItems([[maybe_unused]] nlohmann::json const& arguments)
{
    auto const items = callOnMessageThread(mShouldAbort, [&]()
                                           {
                                               return Operations::getSelectedItems(mInstance.getDocumentAccessor());
                                           });
    return createJsonResponse(items, false);
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getGroupNames(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getGroupNames(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setGroupNames(nlohmann::json const& arguments)
{
    std::map<juce::String, juce::String> names;
    for(auto const& [identifier, group] : getItemsArgument(arguments, "groups", "The 'groups' argument is required and must be an array of objects."))
    {
        names.insert_or_assign(identifier, getArgument<juce::String>(group, "name", "The 'name' field is required and must be a string."));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setGroupNames(mInstance.getDocumentDirector(), names);
                                             });
    return createItemsResponse(results, [&](juce::String const& identifier)
                               {
                                   return juce::String(R"(The group has been renamed "NAME".)").replace("NAME", names.at(identifier));
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getGroupLayouts(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getGroupLayouts(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setGroupLayouts(nlohmann::json const& arguments)
{
    std::map<juce::String, std::vector<juce::String>> layouts;
    for(auto const& [identifier, group] : getItemsArgument(arguments, "groups", "The 'groups' argument is required and must be an array of objects."))
    {
        layouts.insert_or_assign(identifier, getArgument<std::vector<juce::String>>(group, "layout", "The 'layout' field is required and must be an array of strings."));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setGroupLayouts(mInstance.getDocumentDirector(), layouts);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The group layout has been modified.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getGroupVerticalZoom(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getGroupVerticalZoom(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setGroupVerticalZoom(nlohmann::json const& arguments)
{
    auto const visibleRanges = getRangesArgument(arguments, "groups", "The 'groups' argument is required and must be an array of objects.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setGroupVerticalZoom(mInstance.getDocumentAccessor(), visibleRanges);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::createGroups(nlohmann::json const& arguments)
{
    static auto constexpr groupsError = "The 'groups' argument is required and must be an array of objects.";
    std::vector<Operations::GroupCreation> groups;
    for(auto const& group : getArgument<std::vector<nlohmann::json>>(arguments, "groups", groupsError))
    {
        if(!group.is_object())
        {
            throw ExecutionError(groupsError);
        }
        Operations::GroupCreation creation;
        creation.name = getOptionalArgument<juce::String>(group, "name", {}, "The 'name' field must be a string.");
        creation.position = getOptionalPositionArgument(group);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackTypes(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackTypes(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackDescriptions(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackDescriptions(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackParameters(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackParameters(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackVerticalZoom(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackVerticalZoom(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackVerticalZoom(nlohmann::json const& arguments)
{
    auto const visibleRanges = getRangesArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setTrackVerticalZoom(mInstance.getDocumentAccessor(), visibleRanges);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackValueRange(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackValueRange(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackValueRange(nlohmann::json const& arguments)
{
    auto const globalRanges = getRangesArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::setTrackValueRange(mInstance.getDocumentAccessor(), globalRanges);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackInputs(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackInputs(mInstance.getDocumentAccessor(), identifiers);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackIsVisibleInGroup(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackIsVisibleInGroup(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackIsVisibleInGroup(nlohmann::json const& arguments)
{
    std::map<juce::String, bool> visibilities;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        visibilities.insert_or_assign(identifier, getArgument<bool>(track, "visibleInGroup", "The 'visibleInGroup' field is required and must be a boolean."));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackIsVisibleInGroup(mInstance.getDocumentDirector(), visibilities);
                                             });
    return createItemsResponse(results, [&](juce::String const& identifier)
                               {
                                   return juce::String(visibilities.at(identifier) ? "The track is visible in its group." : "The track is hidden in its group.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackExtraThresholds(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackExtraThresholds(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackGraphics(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackGraphics(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackGraphics(nlohmann::json const& arguments)
{
    auto const defaultGraphics = getTrackGraphicsArgument(arguments);
    std::map<juce::String, Operations::TrackGraphics> graphics;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        graphics.insert_or_assign(identifier, getTrackGraphicsArgument(track));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackGraphics(mInstance.getDocumentDirector(), defaultGraphics, graphics);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackExtraThresholds(nlohmann::json const& arguments)
{
    static auto constexpr thresholdsError = "The 'extraThresholds' field must be an array of numbers or null values.";
    // An empty array is a valid value (that removes the thresholds) so the arrays are not parsed as std::optional
    auto const getThresholds = [&](nlohmann::json const& object, std::optional<std::vector<std::optional<float>>> const& defaultValue)
    {
        return object.contains("extraThresholds") ? std::make_optional(getArgument<std::vector<std::optional<float>>>(object, "extraThresholds", thresholdsError)) : defaultValue;
    };
    auto const defaultThresholds = getThresholds(arguments, std::nullopt);
    std::map<juce::String, std::vector<std::optional<float>>> thresholds;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        auto const trackThresholds = getThresholds(track, defaultThresholds);
        if(!trackThresholds.has_value())
        {
            throw ExecutionError("The 'extraThresholds' field is required either at the top-level or per track.");
        }
        thresholds.insert_or_assign(identifier, trackThresholds.value());
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackExtraThresholds(mInstance.getDocumentDirector(), thresholds);
                                             });
    return createItemsResponse(results, []([[maybe_unused]] juce::String const& identifier)
                               {
                                   return juce::String("The extra thresholds have been modified.");
                               });
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::createTracksWithPlugins(nlohmann::json const& arguments)
{
    static auto constexpr tracksError = "The 'tracks' argument is required and must be an array of objects.";
    static auto constexpr groupError = "The 'groupIdentifier' field must be a string.";
    auto const defaultPlugin = getOptionalPluginArgument(arguments);
    auto const defaultGroup = getOptionalArgument<juce::String>(arguments, "groupIdentifier", {}, groupError);
    auto const defaultParameters = getParametersArgument(arguments);
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
        creation.position = getOptionalPositionArgument(track);
        creation.name = getOptionalArgument<std::optional<juce::String>>(track, "name", {}, "The 'name' field must be a string.");
        creation.parameters = getParametersArgument(track);
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
        creation.position = getOptionalPositionArgument(track);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::saveTrackResults(nlohmann::json const& arguments)
{
    using Options = Document::Exporter::Options;
    auto const identifiers = getArgument<std::set<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
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

    auto const directory = getTempExportDirectory().getChildFile(juce::Uuid().toString());
    // As for the export of the application, the export is performed on this thread
    auto const [result, files] = Operations::saveTrackResults(mInstance.getDocumentAccessor(), identifiers, options, directory);
    if(result.failed())
    {
        return createTextResponse(result.getErrorMessage().toStdString(), true);
    }
    nlohmann::json payload;
    payload["directory"] = directory;
    payload["files"] = files;
    return createJsonResponse(payload, false);
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

nlohmann::json Application::Neuralyzer::Mcp::Handlers::getTrackNames(nlohmann::json const& arguments)
{
    auto const identifiers = getArgument<std::vector<juce::String>>(arguments, "identifiers", "The 'identifiers' argument is required and must be an array of strings.");
    return createItemsResponse(callOnMessageThread(mShouldAbort, [&]()
                                                   {
                                                       return Operations::getTrackNames(mInstance.getDocumentAccessor(), identifiers);
                                                   }));
}

nlohmann::json Application::Neuralyzer::Mcp::Handlers::setTrackNames(nlohmann::json const& arguments)
{
    std::map<juce::String, juce::String> names;
    for(auto const& [identifier, track] : getItemsArgument(arguments, "tracks", "The 'tracks' argument is required and must be an array of objects."))
    {
        names.insert_or_assign(identifier, getArgument<juce::String>(track, "name", "The 'name' field is required and must be a string."));
    }
    auto const results = callOnMessageThread(mShouldAbort, [&]()
                                             {
                                                 return Operations::setTrackNames(mInstance.getDocumentDirector(), names);
                                             });
    return createItemsResponse(results, [&](juce::String const& identifier)
                               {
                                   return juce::String(R"(The track has been renamed "NAME".)").replace("NAME", names.at(identifier));
                               });
}

ANALYSE_FILE_END
