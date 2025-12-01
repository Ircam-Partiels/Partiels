#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

juce::File Application::Accessor::getFactoryTemplateFile()
{
    auto const exeFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile);
#if JUCE_MAC
    return exeFile.getParentDirectory().getSiblingFile("Resources").getChildFile("Templates").getChildFile("FactoryTemplate.ptldoc");
#else
    return exeFile.getSiblingFile("Templates").getChildFile("FactoryTemplate.ptldoc");
#endif
}

std::unique_ptr<juce::XmlElement> Application::Accessor::parseXml(juce::XmlElement const& xml, int version)
{
    auto copy = std::make_unique<juce::XmlElement>(xml);
    if(version < 0x10300)
    {
        if(auto* child = copy->getChildByName("defaultTemplateFile"))
        {
            copy->removeChildElement(child, true);
        }
        XmlParser::toXml(*copy.get(), "defaultTemplateFile", juce::File{});
    }
    if(version < 0x20301)
    {
        // Migrate old container format (multiple sibling elements) to new format (parent with children)
        XmlParser::migrateContainerFormat(*copy.get(), "recentlyOpenedFilesList");
        XmlParser::migrateContainerFormat(*copy.get(), "routingMatrix");
    }
    return copy;
}

void Application::Accessor::setRecentlyOpenedFilesList(std::vector<juce::File> const& value, NotificationType notification)
{
    auto const sanitize = [](std::vector<juce::File> const& files)
    {
        std::set<juce::File> duplicates;
        std::vector<juce::File> copy;
        for(auto const& file : files)
        {
            if(file.existsAsFile() && duplicates.insert(file).second)
            {
                copy.push_back(file);
            }
        }
        return copy;
    };
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
}

void Application::Accessor::setDesktopGlobalScaleFactor(float const& value, NotificationType notification)
{
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::desktopGlobalScaleFactor, float>(std::clamp(value, 1.0f, 2.0f), notification);
}

void Application::Accessor::setQuickExportDirectory(juce::File const& value, NotificationType notification)
{
    auto const directory = value.isDirectory() ? value : juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::quickExportDirectory, juce::File>(directory, notification);
}

ANALYSE_FILE_END
