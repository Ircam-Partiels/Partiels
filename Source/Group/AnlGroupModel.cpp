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
    if(version < 0x20300)
    {
        // Migrate old container format (multiple sibling elements) to new format (parent with children)
        auto migrateContainerFormat = [](juce::XmlElement& parent, juce::Identifier const& name)
        {
            auto const* firstChild = parent.getChildByName(name);
            if(firstChild != nullptr && firstChild->hasAttribute("value"))
            {
                auto newElement = std::make_unique<juce::XmlElement>(name);
                while(auto* child = parent.getChildByName(name))
                {
                    auto childCopy = std::make_unique<juce::XmlElement>(*child);
                    parent.removeChildElement(child, true);
                    newElement->addChildElement(childCopy.release());
                }
                parent.addChildElement(newElement.release());
            }
        };
        migrateContainerFormat(*copy.get(), "layout");
    }
    return copy;
}

ANALYSE_FILE_END
