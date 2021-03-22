#include "AnlTrackExporter.h"
#include "AnlTrackRenderer.h"

ANALYSE_FILE_BEGIN

void Analyzer::Exporter::toPreset(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export as preset..."), {}, App::getFileWildCardFor("preset"));
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    juce::TemporaryFile temp(fc.getResult());
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as preset failed!");
    
    auto xml = std::make_unique<juce::XmlElement>("Preset");
    anlWeakAssert(xml != nullptr);
    if(xml == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The analyzer ANLNAME can not be exported as a preset because the analyzer cannot be parsed to XML.").replace("FLNM", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    
    XmlParser::toXml(*xml.get(), "key", accessor.getAttr<AttrType::key>());
    XmlParser::toXml(*xml.get(), "state", accessor.getAttr<AttrType::state>());
    
    auto const file = fc.getResult();
    if(!xml->writeTo(temp.getFile()) && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as a preset because the file cannot FLNAME cannot be written.").replace("FLNM", accessor.getAttr<AttrType::name>().replace("FLNM", file.getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
    
    if(!temp.overwriteTargetFileWithTemporary() && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("FLNM", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

void Analyzer::Exporter::fromPreset(Accessor& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Load from preset..."), {}, App::getFileWildCardFor("preset"));
    if(!fc.browseForFileToOpen())
    {
        return;
    }
    auto const file = fc.getResult();
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as preset failed!");
    
    auto xml = juce::XmlDocument::parse(file);
    if(xml == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The analyzer ANLNAME can not be parse to a preset.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    
    Plugin::Key const key = XmlParser::fromXml(*xml.get(), "key", Plugin::Key());
    if(key != accessor.getAttr<AttrType::key>())
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The analyzer ANLNAME can not be parse to a preset because the key arer not compatible.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    accessor.setAttr<AttrType::state>(XmlParser::fromXml(*xml.get(), "state", accessor.getAttr<AttrType::state>()), NotificationType::synchronous);
}

void Analyzer::Exporter::toTemplate(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export as template..."), {}, App::getFileWildCardFor("template"));
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    juce::TemporaryFile temp(fc.getResult());
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as template failed!");
    
    auto xml = accessor.toXml("Template");
    if(xml == nullptr)
    {
        if(alertType == AlertType::window)
        {
            auto const message = juce::translate("The analyzer ANLNAME can not be exported as a template because the analyzer cannot be parsed to XML.").replace("FLNM", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    
    auto const file = fc.getResult();
    if(!xml->writeTo(temp.getFile()) && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as a template because the file cannot FLNAME cannot be written.").replace("FLNM", accessor.getAttr<AttrType::name>().replace("FLNM", file.getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
    
    if(!temp.overwriteTargetFileWithTemporary() && alertType == AlertType::window)
    {
        auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("FLNM", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

void Analyzer::Exporter::toImage(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export to image"), {}, "*.png;*.jpeg;*.jpg");
    if (!fc.browseForFileToSave(true))
    {
        return;
    }
    
    juce::TemporaryFile temp(fc.getResult());
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as image failed!");
    
    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        if(alertType!= AlertType::window)
        {
            return;
        }
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as image because the file format of FLNM supported.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
        return;
    }
    
    juce::FileOutputStream stream(temp.getFile());
    if(!stream.openedOk())
    {
        if(alertType!= AlertType::window)
        {
            return;
        }
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as image because the output stream of FLNM cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
        return;
    }
    
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const width = static_cast<int>(results.size());
    auto const height = static_cast<int>(results.empty() ? 0 : results[0].values.size());
    if(width < 0 || height < 0)
    {
        return;
    }
    
    auto image = Renderer::createImage(accessor);
    if(!imageFormat->writeImageToStream(image, stream))
    {
        if(alertType!= AlertType::window)
        {
            return;
        }
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as image because the output stream of FLNM cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
        return;
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        if(alertType!= AlertType::window)
        {
            return;
        }
        auto const message = juce::translate("The analysis ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

void Analyzer::Exporter::toCsv(Accessor const& accessor, AlertType const alertType)
{
    juce::FileChooser fc(juce::translate("Export as CVS..."), {}, "*.csv");
    if(!fc.browseForFileToSave(true))
    {
        return;
    }
    
    juce::TemporaryFile temp(fc.getResult());
    juce::FileOutputStream stream(temp.getFile());
    
    auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
    auto const title = juce::translate("Export as CSV failed!");
    
    if(!stream.openedOk())
    {
        auto const message = juce::translate("The analyzer ANLNAME can not be exported as CBV because the output stream of FLNM cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNM", temp.getTargetFile().getFullPathName()));
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
    
    auto const& results = accessor.getAttr<AttrType::results>();
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

void Analyzer::Exporter::toXml(Accessor const& accessor, AlertType const alertType)
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
                auto const message = juce::translate("The analyzer ANLNAME can not be exported as XML because the analyzer cannot be parsed to XML.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
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
            auto const message = juce::translate("The analyzer ANLNAME can not be exported as XML because the analyzer cannot be parsed to XML.").replace("ANLNAME", accessor.getAttr<AttrType::name>());
            juce::AlertWindow::showMessageBox(icon, title, message);
        }
        return;
    }
    auto const& results = accessor.getAttr<AttrType::results>();
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
