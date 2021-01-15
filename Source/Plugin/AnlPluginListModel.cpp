
#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

bool PluginList::Description::operator==(Description const& rhd) const
{
    return name == rhd.name && features == rhd.features && maker == rhd.maker && api == rhd.api && categories == rhd.categories && details == rhd.details;
}

bool PluginList::Description::operator!=(Description const& rhd) const
{
    return !(*this == rhd);
}

template<>
void XmlParser::toXml<PluginList::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, PluginList::Description const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        child->setAttribute("name", value.name);
        child->setAttribute("maker", value.maker);
        child->setAttribute("api", static_cast<int>(value.api));
        child->setAttribute("details", value.details);
        for(auto const& category : value.categories)
        {
            auto subchild = std::make_unique<juce::XmlElement>("category");
            anlWeakAssert(subchild != nullptr);
            if(subchild != nullptr)
            {
                subchild->setAttribute("category", category);
                child->addChildElement(subchild.release());
            }
        }
        for(auto const& feature : value.features)
        {
            auto subchild = std::make_unique<juce::XmlElement>("feature");
            anlWeakAssert(subchild != nullptr);
            if(subchild != nullptr)
            {
                subchild->setAttribute("feature", feature);
                child->addChildElement(subchild.release());
            }
        }
        xml.addChildElement(child.release());
    }
}

template<>
auto XmlParser::fromXml<PluginList::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, PluginList::Description const& defaultValue)
-> PluginList::Description
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    anlWeakAssert(child->hasAttribute("name"));
    anlWeakAssert(child->hasAttribute("maker"));
    anlWeakAssert(child->hasAttribute("api"));
    anlWeakAssert(child->hasAttribute("details"));
    PluginList::Description value;
    value.name = child->getStringAttribute("name", defaultValue.name);
    value.maker = child->getStringAttribute("maker", defaultValue.maker);
    value.api = static_cast<unsigned int>(child->getIntAttribute("api", static_cast<int>(defaultValue.api)));
    value.details = child->getStringAttribute("details", defaultValue.details);
    for(auto* subchild = child->getChildByName("category"); subchild != nullptr; subchild = subchild->getNextElementWithTagName("Category"))
    {
        anlWeakAssert(subchild->hasAttribute("category"));
        if(subchild->hasAttribute("category"))
        {
            value.categories.insert(subchild->getStringAttribute("category"));
        }
    }
    for(auto* subchild = child->getChildByName("feature"); subchild != nullptr; subchild = subchild->getNextElementWithTagName("feature"))
    {
        anlWeakAssert(subchild->hasAttribute("feature"));
        if(subchild->hasAttribute("feature"))
        {
            value.features.push_back(subchild->getStringAttribute("feature"));
        }
    }
    return value;
}

ANALYSE_FILE_END
