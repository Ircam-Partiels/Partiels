#include "AnlTrackExporter.h"
#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

namespace
{
    juce::Result failed(juce::String const& name, juce::String const& format, juce::String const& reason)
    {
        return juce::Result::fail(juce::translate("The export of the track NAME as FORMAT failed because REASON.").replace("NAME", name).replace("FORMAT", format).replace("REASON", reason));
    }

    // clang-format off
    enum class ErrorType
    {
          dataLocked
        , dataInvalid
        , streamAccessFailure
        , streamWritingFailure
        , fileAccessFailure
    };
    // clang-format on

    juce::Result failed(juce::String const& name, juce::String const& format, ErrorType const error, juce::File const& file = {})
    {
        switch(error)
        {
            case ErrorType::dataLocked:
                return failed(name, format, "the result data are being processed");
            case ErrorType::dataInvalid:
                return failed(name, format, "the result data are invalid");
            case ErrorType::streamAccessFailure:
                return failed(name, format, "the output stream cannot be opened");
            case ErrorType::streamWritingFailure:
                return failed(name, format, "the output stream cannot be written");
            case ErrorType::fileAccessFailure:
                return failed(name, format, juce::String("the file FILEPATH cannot be accessed").replace("FILEPATH", file.getFullPathName()));
        }
        return failed(name, format, "of an unknown reason");
    }

    juce::Result aborted(juce::String const& name, juce::String const& format)
    {
        return juce::Result::fail(juce::translate("The export of the track NAME as FORMAT has been aborted.").replace("NAME", name).replace("FORMAT", format));
    }
} // namespace

juce::Result Track::Exporter::fromPreset(Accessor& accessor, juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The file FLNAME cannot be parsed to preset format.").replace("FLNAME", file.getFullPathName()));
    }
    if(!xml->hasTagName("preset"))
    {
        return juce::Result::fail(juce::translate("The preset file FLNAME is not valid.").replace("FLNAME", file.getFullPathName()));
    }

    auto const key = XmlParser::fromXml(*xml.get(), "key", Plugin::Key());
    if(key != accessor.getAttr<AttrType::key>())
    {
        return juce::Result::fail(juce::translate("The plugin key of the preset file FLNAME doesn't correspond to track.").replace("FLNAME", file.getFullPathName()));
    }
    accessor.setAttr<AttrType::state>(XmlParser::fromXml(*xml.get(), "state", accessor.getAttr<AttrType::state>()), NotificationType::synchronous);
    return juce::Result::ok();
}

juce::Result Track::Exporter::toPreset(Accessor const& accessor, juce::File const& file)
{
    auto const name = accessor.getAttr<AttrType::name>();

    auto const title = juce::translate("Export as preset failed!");
    auto xml = std::make_unique<juce::XmlElement>("preset");
    anlWeakAssert(xml != nullptr);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the track cannot be parsed to preset format.").replace("ANLNAME", name));
    }

    XmlParser::toXml(*xml.get(), "key", accessor.getAttr<AttrType::key>());
    XmlParser::toXml(*xml.get(), "state", accessor.getAttr<AttrType::state>());

    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the file FLNAME cannot be written.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Image Track::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, int width, int height)
{
    juce::Image image(juce::Image::PixelFormat::ARGB, width, height, true);
    juce::Graphics g(image);
    g.fillAll(accessor.getAttr<AttrType::colours>().background);
    Renderer::paint(accessor, timeZoomAccessor, g, {0, 0, image.getWidth(), image.getHeight()}, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(Decorator::ColourIds::normalBorderColourId));
    return image;
}

juce::Result Track::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::File const& file, int width, int height, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "image";

    if(width <= 0 || height <= 0)
    {
        return failed(name, format, "the size is invalid");
    }

    juce::TemporaryFile temp(file);
    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        return failed(name, format, "the format is not supported");
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    auto const image = toImage(accessor, timeZoomAccessor, width, height);

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    if(image.isValid())
    {
        juce::FileOutputStream stream(temp.getFile());
        if(!stream.openedOk())
        {
            return failed(name, format, ErrorType::streamAccessFailure);
        }

        if(!imageFormat->writeImageToStream(image, stream))
        {
            return failed(name, format, ErrorType::streamWritingFailure);
        }
    }
    else
    {
        return failed(name, format, "the image cannot be created");
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, Zoom::Range timeRange, std::ostream& stream, bool includeHeader, char separator, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "CSV";

    if(!static_cast<bool>(stream))
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }

    auto const& results = accessor.getAttr<AttrType::results>();

    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }

    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return failed(name, format, ErrorType::dataLocked);
    }

    if(results.isEmpty())
    {
        return failed(name, format, ErrorType::dataInvalid);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    stream << std::fixed;
    stream << std::setprecision(10);

    auto state = false;
    auto const addLine = [&]()
    {
        state = false;
        stream << '\n';
    };

    auto const addColumn = [&](auto const& text)
    {
        if(state)
        {
            stream << separator;
        }
        stream << text;
        state = true;
        return true;
    };

    auto const markers = results.getMarkers();
    auto const points = results.getPoints();
    auto const columns = results.getColumns();
    if(markers != nullptr)
    {
        auto const escapeString = [](juce::String const& s)
        {
            return s.replace("\"", "\\\"").replace("\'", "\\\'").replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n").quoted();
        };

        auto const addChannel = [&](std::vector<Results::Marker> const& channelMarkers)
        {
            if(includeHeader)
            {
                addColumn("TIME");
                addColumn("DURATION");
                addColumn("LABEL");
                addLine();
            }
            auto it = std::lower_bound(channelMarkers.cbegin(), channelMarkers.cend(), timeRange.getStart(), Result::lower_cmp<Results::Marker>);
            while(it != channelMarkers.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                addColumn(std::get<0_z>(*it));
                addColumn(std::get<1_z>(*it));
                addColumn(escapeString(std::get<2_z>(*it)));
                addLine();
                ++it;
            }
        };

        for(size_t i = 0; i < markers->size(); ++i)
        {
            if(shouldAbort)
            {
                return aborted(name, format);
            }

            addChannel(markers->at(i));
            addLine();
        }
    }
    else if(points != nullptr)
    {
        auto const addChannel = [&](std::vector<Results::Point> const& channelPoints)
        {
            if(includeHeader)
            {
                addColumn("TIME");
                addColumn("DURATION");
                addColumn("VALUE");
                addLine();
            }
            auto it = std::lower_bound(channelPoints.cbegin(), channelPoints.cend(), timeRange.getStart(), Result::lower_cmp<Results::Point>);
            while(it != channelPoints.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                addColumn(std::get<0_z>(*it));
                addColumn(std::get<1_z>(*it));
                if(std::get<2_z>(*it).has_value())
                {
                    addColumn(std::get<2_z>(*it).value());
                }
                addLine();
                ++it;
            }
        };

        for(size_t i = 0; i < points->size(); ++i)
        {
            if(shouldAbort)
            {
                return aborted(name, format);
            }

            addChannel(points->at(i));
            addLine();
        }
    }
    else if(columns != nullptr)
    {
        auto const addChannel = [&](std::vector<Results::Column> const& channelColumns)
        {
            auto const numBins = std::accumulate(channelColumns.cbegin(), channelColumns.cend(), 0_z, [](auto const s, auto const& channelColumn)
                                                 {
                                                     return std::max(s, std::get<2>(channelColumn).size());
                                                 });

            if(includeHeader)
            {
                addColumn("TIME");
                addColumn("DURATION");
                for(size_t j = 0; j < numBins; ++j)
                {
                    addColumn("BIN" + juce::String(j));
                }
                addLine();
            }

            auto it = std::lower_bound(channelColumns.cbegin(), channelColumns.cend(), timeRange.getStart(), Result::lower_cmp<Results::Column>);
            while(it != channelColumns.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                addColumn(std::get<0_z>(*it));
                addColumn(std::get<1_z>(*it));
                for(auto const& value : std::get<2_z>(*it))
                {
                    addColumn(value);
                }
                addLine();
                ++it;
            }
        };

        for(size_t i = 0; i < columns->size(); ++i)
        {
            if(shouldAbort)
            {
                return aborted(name, format);
            }

            addChannel(columns->at(i));
            addLine();
        }
    }

    if(!stream.good())
    {
        return failed(name, format, ErrorType::streamWritingFailure);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, Zoom::Range timeRange, juce::File const& file, bool includeHeader, char separator, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "CSV";

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open())
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }
    auto const result = toCsv(accessor, timeRange, stream, includeHeader, separator, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    // This is important on Windows otherwise the temporary file cannot be copied
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, Zoom::Range timeRange, juce::String& string, bool includeHeader, char separator, std::atomic<bool> const& shouldAbort)
{
    std::ostringstream stream;
    auto const result = toCsv(accessor, timeRange, stream, includeHeader, separator, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    string = stream.str();
    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, std::ostream& stream, bool includeDescription, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "JSON";

    if(!static_cast<bool>(stream))
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const description = includeDescription ? accessor.toJson() : nlohmann::json::object();

    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }

    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return failed(name, format, ErrorType::dataLocked);
    }

    if(results.isEmpty())
    {
        return failed(name, format, ErrorType::dataInvalid);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    auto container = nlohmann::json::object();
    auto& json = container["results"];
    if(includeDescription)
    {
        container["track"] = std::move(description);
    }

    auto const markers = results.getMarkers();
    auto const points = results.getPoints();
    auto const columns = results.getColumns();
    if(markers != nullptr)
    {
        for(auto channelIndex = 0_z; channelIndex < markers->size(); ++channelIndex)
        {
            if(channels.empty() || channels.count(channelIndex) > 0_z)
            {
                auto const& channelResults = markers->at(channelIndex);
                if(shouldAbort)
                {
                    return aborted(name, format);
                }

                nlohmann::json cjson;
                auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Marker>);
                while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
                {
                    MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                    nlohmann::json vjson;
                    vjson["time"] = std::get<0_z>(*it);
                    if(std::get<1_z>(*it) > 0.0)
                    {
                        vjson["duration"] = std::get<1_z>(*it);
                    }
                    if(!std::get<2_z>(*it).empty())
                    {
                        vjson["label"] = std::get<2_z>(*it);
                    }
                    cjson.emplace_back(std::move(vjson));
                    ++it;
                }
                json.emplace_back(std::move(cjson));
            }
        }
    }
    else if(points != nullptr)
    {
        for(auto channelIndex = 0_z; channelIndex < points->size(); ++channelIndex)
        {
            if(channels.empty() || channels.count(channelIndex) > 0_z)
            {
                auto const& channelResults = points->at(channelIndex);
                if(shouldAbort)
                {
                    return aborted(name, format);
                }

                nlohmann::json cjson;
                auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Point>);
                while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
                {
                    MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                    nlohmann::json vjson;
                    vjson["time"] = std::get<0_z>(*it);
                    if(std::get<1_z>(*it) > 0.0)
                    {
                        vjson["duration"] = std::get<1_z>(*it);
                    }
                    if(std::get<2_z>(*it).has_value())
                    {
                        vjson["value"] = std::get<2_z>(*it).value();
                    }
                    cjson.emplace_back(std::move(vjson));
                    ++it;
                }
                json.emplace_back(std::move(cjson));
            }
        }
    }
    else if(columns != nullptr)
    {
        for(auto channelIndex = 0_z; channelIndex < columns->size(); ++channelIndex)
        {
            if(channels.empty() || channels.count(channelIndex) > 0_z)
            {
                auto const& channelResults = columns->at(channelIndex);
                nlohmann::json cjson;
                auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Column>);
                while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
                {
                    MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                    if(shouldAbort)
                    {
                        return aborted(name, format);
                    }

                    nlohmann::json vjson;
                    vjson["time"] = std::get<0_z>(*it);
                    if(std::get<1_z>(*it) > 0.0)
                    {
                        vjson["duration"] = std::get<1_z>(*it);
                    }
                    vjson["values"] = std::get<2_z>(*it);
                    cjson.emplace_back(std::move(vjson));
                    ++it;
                }
                json.emplace_back(std::move(cjson));
            }
        }
    }

    stream << container << std::endl;
    if(!stream.good())
    {
        return failed(name, format, ErrorType::streamWritingFailure);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::File const& file, bool includeDescription, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "JSON";

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open())
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }
    auto const result = toJson(accessor, timeRange, channels, stream, includeDescription, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    // This is important on Windows otherwise the temporary file cannot be copied
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, Zoom::Range timeRange, std::set<size_t> const& channels, juce::String& string, bool includeDescription, std::atomic<bool> const& shouldAbort)
{
    std::ostringstream stream;
    auto const result = toJson(accessor, timeRange, channels, stream, includeDescription, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    string = stream.str();
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCue(Accessor const& accessor, Zoom::Range timeRange, std::ostream& stream, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "CUE";

    if(!static_cast<bool>(stream))
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }

    auto const& results = accessor.getAttr<AttrType::results>();

    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }

    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return failed(name, format, ErrorType::dataLocked);
    }

    if(results.isEmpty())
    {
        return failed(name, format, ErrorType::dataInvalid);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    auto const markers = results.getMarkers();
    if(markers == nullptr)
    {
        return failed(name, format, "the result data are not of marker type");
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    auto const to2Digits = [](int value)
    {
        value = std::max(std::min(value, 99), 0);
        auto const v10 = static_cast<int>(value / 10);
        return std::to_string(v10) + std::to_string(value - (v10 * 10));
    };

    auto const timeToString = [&](double time)
    {
        auto const minutes = std::floor(time / 60.0);
        auto const seconds = std::floor(time - (minutes * 60.0));
        auto const frames = std::round((time - (minutes * 60.0) - seconds) * 75.0);
        return to2Digits(static_cast<int>(minutes)) + ":" + to2Digits(static_cast<int>(seconds)) + ":" + to2Digits(static_cast<int>(frames));
    };

    for(auto channelIndex = 0_z; channelIndex < markers->size(); ++channelIndex)
    {
        if(shouldAbort)
        {
            return aborted(name, format);
        }

        auto const& channelResults = markers->at(channelIndex);
        stream << "TITLE CHANNEL_" << to2Digits(static_cast<int>(channelIndex)) << "\n";
        auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Marker>);
        auto frameIndex = std::distance(channelResults.cbegin(), it);
        while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
        {
            MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
            stream << "  "
                   << "TRACK " << to2Digits(static_cast<int>(frameIndex)) << " AUDIO \n";
            stream << "    "
                   << "TITLE \"" << std::get<2_z>(*it) << "\"\n";
            stream << "    "
                   << "INDEX 01 " << timeToString(std::get<0_z>(*it)) << "\n";
            ++it;
            ++frameIndex;
        }
    }

    if(!stream.good())
    {
        return failed(name, format, ErrorType::streamWritingFailure);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    return juce::Result::ok();
}

juce::Result Track::Exporter::toCue(Accessor const& accessor, Zoom::Range timeRange, juce::File const& file, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "CUE";

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open())
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }
    auto const result = toCue(accessor, timeRange, stream, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    // This is important on Windows otherwise the temporary file cannot be copied
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCue(Accessor const& accessor, Zoom::Range timeRange, juce::String& string, std::atomic<bool> const& shouldAbort)
{
    std::ostringstream stream;
    auto const result = toCue(accessor, timeRange, stream, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    string = stream.str();
    return juce::Result::ok();
}

juce::Result Track::Exporter::toBinary(Accessor const& accessor, Zoom::Range timeRange, std::ostream& stream, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "DAT";

    if(!static_cast<bool>(stream))
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }

    auto const& results = accessor.getAttr<AttrType::results>();

    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }

    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return failed(name, format, ErrorType::dataLocked);
    }

    if(results.isEmpty())
    {
        return failed(name, format, ErrorType::dataInvalid);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    auto const markers = results.getMarkers();
    auto const points = results.getPoints();
    auto const columns = results.getColumns();
    if(markers != nullptr)
    {
        stream.write("PTLMKS", 6 * sizeof(char));
        for(auto const& channelResults : *markers)
        {
            if(shouldAbort)
            {
                return aborted(name, format);
            }

            auto const numChannels = static_cast<uint64_t>(channelResults.size());
            stream.write(reinterpret_cast<char const*>(&numChannels), sizeof(numChannels));
            auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Marker>);
            while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                stream.write(reinterpret_cast<char const*>(&std::get<0_z>(*it)), sizeof(std::get<0_z>(*it)));
                stream.write(reinterpret_cast<char const*>(&std::get<1_z>(*it)), sizeof(std::get<1_z>(*it)));
                auto const length = static_cast<uint64_t>(std::get<2_z>(*it).size());
                stream.write(reinterpret_cast<char const*>(&length), sizeof(length));
                stream.write(std::get<2_z>(*it).c_str(), static_cast<long>(sizeof(char) * std::get<2_z>(*it).size()));
                ++it;
            }
        }
    }
    else if(points != nullptr)
    {
        stream.write("PTLPTS", 6 * sizeof(char));
        for(auto const& channelResults : *points)
        {
            if(shouldAbort)
            {
                return aborted(name, format);
            }

            auto const numChannels = static_cast<uint64_t>(channelResults.size());
            stream.write(reinterpret_cast<char const*>(&numChannels), sizeof(numChannels));
            auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Point>);
            while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                stream.write(reinterpret_cast<char const*>(&std::get<0_z>(*it)), sizeof(std::get<0_z>(*it)));
                stream.write(reinterpret_cast<char const*>(&std::get<1_z>(*it)), sizeof(std::get<1_z>(*it)));
                auto const hasValue = std::get<2_z>(*it).has_value();
                stream.write(reinterpret_cast<char const*>(&hasValue), sizeof(hasValue));
                if(hasValue)
                {
                    auto const& value = std::get<2_z>(*it).value();
                    stream.write(reinterpret_cast<char const*>(&value), sizeof(value));
                }
                ++it;
            }
        }
    }
    else if(columns != nullptr)
    {
        stream.write("PTLCLS", 6 * sizeof(char));
        for(auto const& channelResults : *columns)
        {
            auto const numChannels = static_cast<uint64_t>(channelResults.size());
            stream.write(reinterpret_cast<char const*>(&numChannels), sizeof(numChannels));
            auto it = std::lower_bound(channelResults.cbegin(), channelResults.cend(), timeRange.getStart(), Result::lower_cmp<Results::Column>);
            while(it != channelResults.cend() && std::get<0_z>(*it) <= timeRange.getEnd())
            {
                MiscWeakAssert(std::get<0_z>(*it) >= timeRange.getStart());
                if(shouldAbort)
                {
                    return aborted(name, format);
                }

                stream.write(reinterpret_cast<char const*>(&std::get<0_z>(*it)), sizeof(std::get<0_z>(*it)));
                stream.write(reinterpret_cast<char const*>(&std::get<1_z>(*it)), sizeof(std::get<1_z>(*it)));
                auto const numBins = static_cast<uint64_t>(std::get<2_z>(*it).size());
                stream.write(reinterpret_cast<char const*>(&numBins), sizeof(numBins));
                stream.write(reinterpret_cast<char const*>(std::get<2_z>(*it).data()), static_cast<long>(sizeof(*std::get<2_z>(*it).data()) * numBins));
                ++it;
            }
        }
    }

    if(!stream.good())
    {
        return failed(name, format, ErrorType::streamWritingFailure);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    return juce::Result::ok();
}

juce::Result Track::Exporter::toBinary(Accessor const& accessor, Zoom::Range timeRange, juce::File const& file, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "DAT";

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open())
    {
        return failed(name, format, ErrorType::streamAccessFailure);
    }
    auto const result = toBinary(accessor, timeRange, stream, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    // This is important on Windows otherwise the temporary file cannot be copied
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toBinary(Accessor const& accessor, Zoom::Range timeRange, juce::String& string, std::atomic<bool> const& shouldAbort)
{
    std::ostringstream stream;
    auto const result = toBinary(accessor, timeRange, stream, shouldAbort);
    if(result.failed())
    {
        return result;
    }
    string = stream.str();
    return juce::Result::ok();
}

juce::Result Track::Exporter::toSdif(Accessor const& accessor, Zoom::Range timeRange, juce::File const& file, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName, std::atomic<bool> const& shouldAbort)
{
    auto const name = accessor.getAttr<AttrType::name>();
    auto constexpr format = "SDIF";

    auto const& results = accessor.getAttr<AttrType::results>();

    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }
    if(timeRange.isEmpty())
    {
        timeRange = {std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max()};
    }

    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return failed(name, format, ErrorType::dataLocked);
    }

    if(results.isEmpty())
    {
        return failed(name, format, ErrorType::dataInvalid);
    }

    if(shouldAbort)
    {
        return aborted(name, format);
    }

    juce::TemporaryFile temp(file);

    if(auto const markers = results.getMarkers())
    {
        auto const result = SdifConverter::write(temp.getFile(), timeRange, frameId, matrixId, columnName, *markers.get(), [&]()
                                                 {
                                                     return shouldAbort == false;
                                                 });
        if(result.failed())
        {
            return result;
        }
    }
    else if(auto const points = results.getPoints())
    {
        auto const result = SdifConverter::write(temp.getFile(), timeRange, frameId, matrixId, columnName, *points.get(), [&]()
                                                 {
                                                     return shouldAbort == false;
                                                 });
        if(result.failed())
        {
            return result;
        }
    }
    else if(auto const columns = results.getColumns())
    {
        auto const result = SdifConverter::write(temp.getFile(), timeRange, frameId, matrixId, columnName, *columns.get(), [&]()
                                                 {
                                                     return shouldAbort == false;
                                                 });
        if(result.failed())
        {
            return result;
        }
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return failed(name, format, ErrorType::fileAccessFailure, file);
    }
    return juce::Result::ok();
}

juce::File Track::Exporter::getConsolidatedFile(Accessor const& accessor, juce::File const& directory)
{
    auto const fileName = accessor.getAttr<AttrType::identifier>() + "-" + accessor.getAttr<AttrType::file>().commit;
    return directory.getChildFile(fileName + ".dat");
}

juce::Result Track::Exporter::consolidateInDirectory(Accessor const& accessor, juce::File const& directory)
{
    auto const directoryResult = directory.createDirectory();
    if(directoryResult.failed())
    {
        return directoryResult;
    }

    auto const currentFile = accessor.getAttr<AttrType::file>().file;
    auto const newFile = getConsolidatedFile(accessor, directory);
    if(currentFile == newFile)
    {
        return juce::Result::ok();
    }
    else if(currentFile.getFileName() == newFile.getFileName())
    {
        if(currentFile.copyFileTo(newFile))
        {
            return juce::Result::ok();
        }
        return juce::Result::fail(juce::translate("Cannot copy from SRCFILE to DSTFILE!").replace(currentFile.getFullPathName(), newFile.getFullPathName()));
    }
    else
    {
        std::atomic<bool> shouldAbort = false;
        return toBinary(accessor, {}, newFile, shouldAbort);
    }
}

ANALYSE_FILE_END
