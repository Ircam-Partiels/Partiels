
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

template <>
void XmlParser::toXml<Application::ExportOptions>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::ExportOptions const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "format", value.format);
        toXml(*child, "useGroupOverview", value.useGroupOverview);
        toXml(*child, "useAutoSize", value.useAutoSize);
        toXml(*child, "imageWidth", value.imageWidth);
        toXml(*child, "imageHeight", value.imageHeight);
        toXml(*child, "includeHeaderRaw", value.includeHeaderRaw);
        toXml(*child, "ignoreGridResults", value.ignoreGridResults);
        toXml(*child, "columnSeparator", value.columnSeparator);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Application::ExportOptions>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Application::ExportOptions const& defaultValue)
    -> Application::ExportOptions
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Application::ExportOptions value;
    value.format = fromXml(*child, "format", defaultValue.format);
    value.useGroupOverview = fromXml(*child, "useGroupOverview", defaultValue.useGroupOverview);
    value.useAutoSize = fromXml(*child, "useAutoSize", defaultValue.useAutoSize);
    value.imageWidth = fromXml(*child, "imageWidth", defaultValue.imageWidth);
    value.imageHeight = fromXml(*child, "imageHeight", defaultValue.imageHeight);
    value.includeHeaderRaw = fromXml(*child, "includeHeaderRaw", defaultValue.includeHeaderRaw);
    value.ignoreGridResults = fromXml(*child, "ignoreGridResults", defaultValue.ignoreGridResults);
    value.columnSeparator = fromXml(*child, "columnSeparator", defaultValue.columnSeparator);
    return value;
}

ANALYSE_FILE_END
