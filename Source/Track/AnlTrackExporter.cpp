#include "AnlTrackExporter.h"
#include "../Transport/AnlTransportModel.h"
#include "AnlTrackPlot.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

juce::Result Track::Exporter::fromPreset(Accessor& accessor, juce::File const& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The file FLNAME cannot be parsed to preset format.").replace("FLNAME", file.getFullPathName()));
    }

    if(!xml->hasAttribute("key"))
    {
        return juce::Result::fail(juce::translate("The preset file FLNAME doesn't contain a plugin key.").replace("FLNAME", file.getFullPathName()));
    }
    if(!xml->hasAttribute("state"))
    {
        return juce::Result::fail(juce::translate("The preset file FLNAME doesn't contain a saved state.").replace("FLNAME", file.getFullPathName()));
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
    auto const title = juce::translate("Export as preset failed!");
    auto xml = std::make_unique<juce::XmlElement>("Preset");
    anlWeakAssert(xml != nullptr);
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the track cannot be parsed to preset format.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }

    XmlParser::toXml(*xml.get(), "key", accessor.getAttr<AttrType::key>());
    XmlParser::toXml(*xml.get(), "state", accessor.getAttr<AttrType::state>());

    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as a preset because the file FLNAME cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height)
{
    if(width <= 0 || height <= 0)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because image size is not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }
    juce::TemporaryFile temp(file);

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    juce::FileOutputStream stream(temp.getFile());
    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    Transport::Accessor transportAccessor;
    Plot plot(accessor, timeZoomAccessor, transportAccessor);
    plot.setSize(width, height);
    auto const image = plot.createComponentSnapshot({0, 0, width, height});
    if(!imageFormat->writeImageToStream(image, stream))
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toCsv(Accessor const& accessor, juce::File const& file)
{
    auto const resultsPtr = accessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the results are not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("ANLNAME", accessor.getAttr<AttrType::name>())));
    }

    juce::TemporaryFile temp(file);
    juce::FileOutputStream stream(temp.getFile());

    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
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
            stream << ',';
        }
        stream << text;
        state = true;
        return true;
    };

    auto toSeconds = [](Vamp::RealTime const& timestamp)
    {
        return static_cast<double>(timestamp.sec) + static_cast<double>(timestamp.msec()) / 1000.0;
    };

    addColumn("ROW");
    addColumn("TIME");
    addColumn("DURATION");
    addColumn("LABEL");

    auto const& results = *resultsPtr;
    for(size_t i = 0; i < results.front().values.size(); ++i)
    {
        addColumn("BIN " + juce::String(i));
    }
    addLine();
    for(size_t i = 0; i < results.size(); ++i)
    {
        auto const& result = results[i];
        addColumn(juce::String(i));
        addColumn(result.hasTimestamp ? juce::String(toSeconds(result.timestamp)) : "0");
        addColumn(result.hasDuration ? juce::String(toSeconds(result.duration)) : "0");
        addColumn(result.label.empty() ? "\"\"" : juce::String(result.label));
        for(auto const& value : result.values)
        {
            addColumn(juce::String(value));
        }
        addLine();
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toXml(Accessor const& accessor, juce::File const& file)
{
    auto const resultsPtr = accessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as XML because the results are not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("ANLNAME", accessor.getAttr<AttrType::name>())));
    }

    auto xml = std::make_unique<juce::XmlElement>("results");
    if(xml == nullptr)
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as XML because the results cannot be parsed to XML format.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }
    XmlParser::toXml(*xml.get(), "row", *resultsPtr);

    if(!xml->writeTo(file))
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as XML because the file FLNAME cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

juce::Result Track::Exporter::toJson(Accessor const& accessor, juce::File const& file)
{
    auto const resultsPtr = accessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as JSON because the results are not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("ANLNAME", accessor.getAttr<AttrType::name>())));
    }

    juce::TemporaryFile temp(file);
    juce::FileOutputStream stream(temp.getFile());

    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be exported as CSV because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    auto resultToVar = [](juce::DynamicObject& obj, Plugin::Result const& result)
    {
        if(result.hasTimestamp)
        {
            obj.setProperty("time", Tools::realTimeToSeconds(result.timestamp));
        }
        if(result.hasDuration)
        {
            obj.setProperty("duration", Tools::realTimeToSeconds(result.duration));
        }
        if(!result.label.empty())
        {
            obj.setProperty("label", juce::String(result.label));
        }
        if(result.values.size() == 1_z)
        {
            obj.setProperty("value", result.values[0]);
        }
        else if(result.values.size() > 1_z)
        {
            juce::var results;
            for(auto const& value : result.values)
            {
                results.append(value);
            }
            obj.setProperty("values", results);
        }
    };
    auto const& results = *resultsPtr;
    juce::Array<juce::var> array;
    array.ensureStorageAllocated(static_cast<int>(results.size()));
    for(auto const& result : results)
    {
        juce::var v(new juce::DynamicObject());
        resultToVar(*v.getDynamicObject(), result);
        array.add(v);
    }
    juce::var const json(array);
    json.writeToStream(stream);

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The results of the track ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
