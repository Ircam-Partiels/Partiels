#include "AnlTrackExporter.h"
#include "../Transport/AnlTransportModel.h"
#include "AnlTrackPlot.h"

ANALYSE_FILE_BEGIN

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

void Track::Exporter::toCsv(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export as CVS..."), {}, "*.csv");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }

    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as CSV failed!");

    auto const resultsPtr = accessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The track ANLNAME can not be exported as CSV because the results are not computed yet.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }

    juce::TemporaryFile temp(fc.getResult());
    juce::FileOutputStream stream(temp.getFile());

    if(!stream.openedOk())
    {
        auto const message = juce::translate("The track ANLNAME can not be exported as CSV because the output stream of FLNM cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
        return;
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

    if(!temp.overwriteTargetFileWithTemporary() && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("FLNM", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

void Track::Exporter::toXml(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export as XML..."), {}, "*.xml");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }

    juce::TemporaryFile temp(fc.getResult());

    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as XML failed!");

    auto toSeconds = [](Vamp::RealTime const& timestamp)
    {
        return static_cast<double>(timestamp.sec) + static_cast<double>(timestamp.msec()) / 1000.0;
    };

    auto toXml = [&](size_t row, Plugin::Result const& result)
    {
        auto child = std::make_unique<juce::XmlElement>("result");
        if(child == nullptr)
        {
            if(alertType == AlertType::window)
            {
                auto const message = juce::translate("The track ANLNAME can not be exported as XML because the track cannot be parsed to XML.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
                juce::AlertWindow::showMessageBox(icon, title, message);
            }
            return child;
        }

        XmlParser::toXml(*child, "row", row);
        XmlParser::toXml(*child, "time", result.hasTimestamp ? toSeconds(result.timestamp) : 0.0);
        XmlParser::toXml(*child, "duration", result.hasDuration ? toSeconds(result.duration) : 0.0);
        XmlParser::toXml(*child, "label", result.label);
        XmlParser::toXml(*child, "values", result.values);

        return child;
    };

    auto element = std::make_unique<juce::XmlElement>("partiels");
    if(element == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The track ANLNAME can not be exported as XML because the track cannot be parsed to XML.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }

    auto const resultsPtr = accessor.getAttr<AttrType::results>();
    if(resultsPtr == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The track ANLNAME can not be exported as XML because the results are not computed yet.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }

    auto const& results = *resultsPtr;
    for(size_t i = 0; i < results.size(); ++i)
    {
        element->addChildElement(toXml(i, results[i]).release());
    }

    if(!element->writeTo(temp.getFile()))
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", temp.getTargetFile().getFullPathName()));
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }

    if(!temp.overwriteTargetFileWithTemporary() && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

ANALYSE_FILE_END
