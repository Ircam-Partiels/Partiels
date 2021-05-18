#include "AnlTrackExporter.h"
#include "../Transport/AnlTransportModel.h"
#include "AnlTrackPlot.h"
#include "AnlTrackTools.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#include <json/json.hpp>
#pragma GCC diagnostic pop
#include <fstream>

ANALYSE_FILE_BEGIN

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

juce::Result Track::Exporter::toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(width <= 0 || height <= 0)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because image size is not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }
    juce::TemporaryFile temp(file);

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    juce::FileOutputStream stream(temp.getFile());
    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    auto getImage = [&]()
    {
        if(!lock.tryEnter())
        {
            return juce::Image{};
        }
        Transport::Accessor transportAccessor;
        Plot plot(accessor, timeZoomAccessor, transportAccessor);
        plot.setSize(width, height);
        return plot.createComponentSnapshot({0, 0, width, height});
    };

    auto const image = getImage();
    if(!imageFormat->writeImageToStream(image, stream))
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be written.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, juce::File const& file, bool includeHeader, char separator)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const results = accessor.getAttr<AttrType::results>();
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(results.isEmpty())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the results are not valid.").replace("ANLNAME", name));
    }

    juce::TemporaryFile temp(file);
    juce::FileOutputStream stream(temp.getFile());

    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    auto state = false;
    auto addLine = [&]()
    {
        state = false;
        stream << '\n';
    };

    auto addColumn = [&](juce::StringRef const& text)
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
    if(markers != nullptr && !markers->empty())
    {
        if(includeHeader)
        {
            addColumn("ROW");
            addColumn("TIME");
            addColumn("DURATION");
            addColumn("LABEL");
            addLine();
        }

        auto const& channel = markers->at(0);
        for(size_t i = 0; i < channel.size(); ++i)
        {
            addColumn(juce::String(i));
            addColumn(juce::String(std::get<0>(channel[i])));
            addColumn(juce::String(std::get<1>(channel[i])));
            addColumn(std::get<2>(channel[i]).empty() ? "\"\"" : juce::String(std::get<2>(channel[i])));
            addLine();
        }
    }
    else if(points != nullptr && !points->empty())
    {
        if(includeHeader)
        {
            addColumn("ROW");
            addColumn("TIME");
            addColumn("DURATION");
            addColumn("VALUE");
            addLine();
        }

        auto const& channel = points->at(0);
        for(size_t i = 0; i < channel.size(); ++i)
        {
            addColumn(juce::String(i));
            addColumn(juce::String(std::get<0>(channel[i])));
            addColumn(juce::String(std::get<1>(channel[i])));
            addColumn(std::get<2>(channel[i]).has_value() ? "\"\"" : juce::String(*std::get<2>(channel[i])));
            addLine();
        }
    }
    else if(columns != nullptr && !columns->empty())
    {
        auto const& channel = columns->at(0);
        if(includeHeader)
        {
            addColumn("ROW");
            addColumn("TIME");
            addColumn("DURATION");
            for(size_t i = 0; i < std::get<2>(channel.front()).size(); ++i)
            {
                addColumn("BIN " + juce::String(i));
            }
            addLine();
        }

        for(size_t i = 0; i < channel.size(); ++i)
        {
            addColumn(juce::String(i));
            addColumn(juce::String(std::get<0>(channel[i])));
            addColumn(juce::String(std::get<1>(channel[i])));
            for(auto const& value : std::get<2>(channel[i]))
            {
                addColumn(juce::String(value));
            }
            addLine();
        }
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, juce::File const& file)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const results = accessor.getAttr<AttrType::results>();
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(results.isEmpty())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as JSON because the results are not valid.").replace("ANLNAME", name).replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }

    nlohmann::json json;

    auto const markers = results.getMarkers();
    auto const points = results.getPoints();
    auto const columns = results.getColumns();
    if(markers != nullptr && !markers->empty())
    {
        auto const& channel = markers->at(0);
        for(auto const& result : channel)
        {
            nlohmann::json vjson;
            vjson["time"] = std::get<0>(result);
            if(std::get<1>(result) > 0.0)
            {
                vjson["duration"] = std::get<1>(result);
            }
            if(!std::get<2>(result).empty())
            {
                vjson["label"] = std::get<2>(result);
            }
            json.emplace_back(vjson);
        }
    }
    else if(points != nullptr && !points->empty())
    {
        auto const& channel = points->at(0);
        for(auto const& result : channel)
        {
            nlohmann::json vjson;
            vjson["time"] = std::get<0>(result);
            if(std::get<1>(result) > 0.0)
            {
                vjson["duration"] = std::get<1>(result);
            }
            if(std::get<2>(result).has_value())
            {
                vjson["value"] = *std::get<2>(result);
            }
            json.emplace_back(vjson);
        }
    }
    else if(columns != nullptr && !columns->empty())
    {
        auto const& channel = columns->at(0);
        for(auto const& result : channel)
        {
            nlohmann::json vjson;
            vjson["time"] = std::get<0>(result);
            if(std::get<1>(result) > 0.0)
            {
                vjson["duration"] = std::get<1>(result);
            }
            vjson["values"] = std::get<2>(result);
            json.emplace_back(vjson);
        }
    }

    juce::TemporaryFile temp(file);
    std::ofstream stream(temp.getFile().getFullPathName().toStdString());
    if(!stream.is_open() || !stream.good())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as JSON because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    stream << json << std::endl;
    stream.close();
    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
