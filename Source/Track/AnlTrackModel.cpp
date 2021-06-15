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

template <>
void XmlParser::toXml<Track::Results>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Results const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "file", value.file.getFullPathName());
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::Results>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Results const& defaultValue)
    -> Track::Results
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::Results value = defaultValue;
    value.file = fromXml(*child, "file", defaultValue.file);
    return value;
}

void Track::to_json(nlohmann::json& j, Results const& results)
{
    j["file"] = results.file;
}

void Track::from_json(nlohmann::json const& j, Results& results)
{
    j.at("file").get_to(results.file);
}

void Track::to_json(nlohmann::json& j, ColourSet const& colourSet)
{
    j["map"] = colourSet.map;
    j["background"] = colourSet.background;
    j["foreground"] = colourSet.foreground;
    j["text"] = colourSet.text;
    j["shadow"] = colourSet.shadow;
    j["grid"] = colourSet.grid;
}

void Track::from_json(nlohmann::json const& j, ColourSet& colourSet)
{
    j.at("map").get_to(colourSet.map);
    j.at("background").get_to(colourSet.background);
    j.at("foreground").get_to(colourSet.foreground);
    j.at("text").get_to(colourSet.text);
    j.at("shadow").get_to(colourSet.shadow);
    j.at("grid").get_to(colourSet.grid);
}

ANALYSE_FILE_END
