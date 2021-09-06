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
    return value;
}

void Track::to_json(nlohmann::json& j, ColourSet const& colourSet)
{
    j["map"] = colourSet.map;
    j["background"] = colourSet.background;
    j["foreground"] = colourSet.foreground;
    j["text"] = colourSet.text;
    j["shadow"] = colourSet.shadow;
}

void Track::from_json(nlohmann::json const& j, ColourSet& colourSet)
{
    colourSet.map = j.value("map", colourSet.map);
    colourSet.background = j.value("background", colourSet.background);
    colourSet.foreground = j.value("foreground", colourSet.foreground);
    colourSet.text = j.value("text", colourSet.text);
    colourSet.shadow = j.value("shadow", colourSet.shadow);
}

ANALYSE_FILE_END
