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
