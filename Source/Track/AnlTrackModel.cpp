#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::XmlElement> Track::Accessor::parseXml(juce::XmlElement const& xml, int version)
{
    auto copy = std::make_unique<juce::XmlElement>(xml);
    if(copy != nullptr)
    {
        if(version <= 0x8)
        {
            auto* child = copy->getChildByName("results");
            if(child != nullptr)
            {
                auto const file = XmlParser::fromXml(*child, "file", juce::File{});
                XmlParser::toXml(*copy.get(), "file", file);
            }
        }
        if(version <= 0x9)
        {
            FileInfo const fileInfo{XmlParser::fromXml(*copy.get(), "file", juce::File{})};
            if(auto* child = copy->getChildByName("file"))
            {
                copy->removeChildElement(child, true);
            }
            XmlParser::toXml(*copy.get(), "file", fileInfo);
        }
        if(version <= 0x10300)
        {
            Plugin::Key key{XmlParser::fromXml(*copy.get(), "key", Plugin::Key{})};
            if(key.identifier == "supervp:superwaveform")
            {
                if(auto* child = copy->getChildByName("key"))
                {
                    copy->removeChildElement(child, true);
                }
                if(auto* child = copy->getChildByName("description"))
                {
                    copy->removeChildElement(child, true);
                }
                key.identifier = "partiels-vamp-plugins:partielswaveform";
                key.feature = "peaks";
                XmlParser::toXml(*copy.get(), "key", key);
            }
        }
        if(version <= 0x20002)
        {
            if(auto* child = copy->getChildByName("colours"))
            {
                auto const foreground = XmlParser::fromXml(*child, "foreground", juce::Colour());
                XmlParser::toXml(*child, "duration", foreground.withAlpha(0.4f));
            }
        }
        if(version <= 0x20201)
        {
            GraphicsSettings settings;
            // Migrate old individual graphic attributes to graphicsSettings (for backward compatibility)
            // and remove old individual graphic attributes
            if(auto* child = copy->getChildByName("colours"))
            {
                settings.colours = XmlParser::fromXml(*copy.get(), "colours", settings.colours);
                copy->removeChildElement(child, true);
            }
            if(auto* child = copy->getChildByName("font"))
            {
                settings.font = XmlParser::fromXml(*copy.get(), "font", settings.font);
                copy->removeChildElement(child, true);
            }
            if(auto* child = copy->getChildByName("lineWidth"))
            {
                settings.lineWidth = XmlParser::fromXml(*copy.get(), "lineWidth", settings.lineWidth);
                copy->removeChildElement(child, true);
            }
            if(auto* child = copy->getChildByName("unit"))
            {
                settings.unit = XmlParser::fromXml(*copy.get(), "unit", settings.unit);
                copy->removeChildElement(child, true);
            }
            if(auto* child = copy->getChildByName("labelLayout"))
            {
                settings.labelLayout = XmlParser::fromXml(*copy.get(), "labelLayout", settings.labelLayout);
                copy->removeChildElement(child, true);
            }
            // Add new graphicsSettings attribute
            XmlParser::toXml(*copy.get(), "graphicsSettings", settings);
        }
    }
    return copy;
}

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
        toXml(*child, "duration", value.duration);
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
    value.duration = fromXml(*child, "duration", defaultValue.duration);
    value.text = fromXml(*child, "text", defaultValue.text);
    value.shadow = fromXml(*child, "shadow", defaultValue.shadow);
    return value;
}

void Track::to_json(nlohmann::json& j, ColourSet const& colourSet)
{
    j["map"] = colourSet.map;
    j["background"] = colourSet.background;
    j["foreground"] = colourSet.foreground;
    j["duration"] = colourSet.duration;
    j["text"] = colourSet.text;
    j["shadow"] = colourSet.shadow;
}

void Track::from_json(nlohmann::json const& j, ColourSet& colourSet)
{
    colourSet.map = j.value("map", colourSet.map);
    colourSet.background = j.value("background", colourSet.background);
    colourSet.foreground = j.value("foreground", colourSet.foreground);
    colourSet.duration = j.value("duration", colourSet.duration);
    colourSet.text = j.value("text", colourSet.text);
    colourSet.shadow = j.value("shadow", colourSet.shadow);
}

template <>
void XmlParser::toXml<Track::LabelLayout>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::LabelLayout const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "position", value.position);
        toXml(*child, "justification", value.justification);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::LabelLayout>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::LabelLayout const& defaultValue)
    -> Track::LabelLayout
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::LabelLayout value;
    value.position = fromXml(*child, "position", defaultValue.position);
    value.justification = fromXml(*child, "justification", defaultValue.justification);
    return value;
}

void Track::to_json(nlohmann::json& j, LabelLayout const& labelLayout)
{
    j["position"] = labelLayout.position;
    j["justification"] = labelLayout.justification;
}

void Track::from_json(nlohmann::json const& j, LabelLayout& labelLayout)
{
    labelLayout.position = j.value("position", labelLayout.position);
    labelLayout.justification = j.value("justification", labelLayout.justification);
}

template <>
void XmlParser::toXml<Track::GraphicsSettings>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::GraphicsSettings const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "colours", value.colours);
        toXml(*child, "font", value.font);
        toXml(*child, "lineWidth", value.lineWidth);
        toXml(*child, "unit", value.unit);
        toXml(*child, "labelLayout", value.labelLayout);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Track::GraphicsSettings>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::GraphicsSettings const& defaultValue)
    -> Track::GraphicsSettings
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::GraphicsSettings value;
    value.colours = fromXml(*child, "colours", defaultValue.colours);
    value.font = fromXml(*child, "font", defaultValue.font);
    value.lineWidth = fromXml(*child, "lineWidth", defaultValue.lineWidth);
    value.unit = fromXml(*child, "unit", defaultValue.unit);
    value.labelLayout = fromXml(*child, "labelLayout", defaultValue.labelLayout);
    return value;
}

void Track::to_json(nlohmann::json& j, GraphicsSettings const& settings)
{
    Track::to_json(j["colours"], settings.colours);
    j["font"] = settings.font;
    j["lineWidth"] = settings.lineWidth;
    j["unit"] = settings.unit;
    Track::to_json(j["labelLayout"], settings.labelLayout);
}

void Track::from_json(nlohmann::json const& j, GraphicsSettings& settings)
{
    if(j.contains("colours"))
    {
        Track::from_json(j["colours"], settings.colours);
    }
    settings.font = j.value("font", settings.font);
    settings.lineWidth = j.value("lineWidth", settings.lineWidth);
    settings.unit = j.value("unit", settings.unit);
    if(j.contains("labelLayout"))
    {
        Track::from_json(j["labelLayout"], settings.labelLayout);
    }
}

ANALYSE_FILE_END
