#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

template <>
void XmlParser::toXml<Track::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::ColourSet const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "map", value.map);
        toXml(*child, "background", value.background);
        toXml(*child, "foreground", value.foreground);
        toXml(*child, "text", value.text);
        toXml(*child, "shadow", value.shadow);
        toXml(*child, "grid", value.grid);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::ColourSet const& defaultValue)
    -> Track::ColourSet
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::ColourSet value;
    value.map = fromXml(*child, "map", defaultValue.map);
    value.background = fromXml(*child, "background", defaultValue.background);
    value.foreground = fromXml(*child, "foreground", defaultValue.foreground);
    value.text = fromXml(*child, "text", defaultValue.text);
    value.shadow = fromXml(*child, "shadow", defaultValue.shadow);
    value.grid = fromXml(*child, "grid", defaultValue.grid);
    return value;
}

ANALYSE_FILE_END
