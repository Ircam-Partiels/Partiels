#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::XmlElement> Group::Accessor::parseXml(juce::XmlElement const& xml, int version)
{
    auto copy = std::make_unique<juce::XmlElement>(xml);
    if(copy != nullptr)
    {
        if(version <= 0x20001)
        {
            if(copy->hasAttribute("zoomid"))
            {
                auto const identifier = XmlParser::fromXml(*copy.get(), "zoomid", juce::String{});
                copy->removeAttribute("zoomid");
                XmlParser::toXml(*copy.get(), "referenceid", identifier);
            }
        }
    }
    return copy;
}

ANALYSE_FILE_END
