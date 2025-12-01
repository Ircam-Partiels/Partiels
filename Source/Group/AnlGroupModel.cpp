#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::XmlElement> Group::Accessor::parseXml(juce::XmlElement const& xml, int version)
{
    auto copy = std::make_unique<juce::XmlElement>(xml);
    if(version <= 0x20001)
    {
        if(copy->hasAttribute("zoomid"))
        {
            auto const identifier = XmlParser::fromXml(*copy.get(), "zoomid", juce::String{});
            copy->removeAttribute("zoomid");
            XmlParser::toXml(*copy.get(), "referenceid", identifier);
        }
    }
    if(version < 0x20301)
    {
        // Migrate old container format (multiple sibling elements) to new format (parent with children)
        XmlParser::migrateContainerFormat(*copy.get(), "layout");
    }
    return copy;
}

ANALYSE_FILE_END
