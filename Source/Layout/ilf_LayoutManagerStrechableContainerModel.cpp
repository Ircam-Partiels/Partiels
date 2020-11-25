
#include "ilf_LayoutManagerStrechableContainerModel.h"

ILF_WARNING_BEGIN
ILF_NAMESPACE_BEGIN

LayoutManager::Strechable::Container::Model LayoutManager::Strechable::Container::Model::fromXml(juce::XmlElement const& xml)
{
    ilfWeakAssert(xml.hasTagName("LayoutManager::Strechable::Container::Model"));
    if(!xml.hasTagName("LayoutManager::Strechable::Container::Model"))
    {
        return {};
    }
    
    Model model;
    model.sizes.reserve(static_cast<size_t>(xml.getNumChildElements()));
    for(auto child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        ilfWeakAssert(child->hasAttribute("size"));
        model.sizes.push_back(child->getIntAttribute("size", 0));
    }
    return model;
}

std::unique_ptr<juce::XmlElement> LayoutManager::Strechable::Container::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("LayoutManager::Strechable::Container::Model");
    ilfWeakAssert(xml != nullptr);
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    for(auto const& size : sizes)
    {
        auto child = std::make_unique<juce::XmlElement>("Size");
        ilfWeakAssert(child != nullptr);
        if(child != nullptr)
        {
            child->setAttribute("size", size);
            xml->addChildElement(child.release());
        }
    }
    return xml;
}

bool LayoutManager::Strechable::Container::Model::operator!=(Model const& other) const
{
    return sizes != other.sizes;
}

bool LayoutManager::Strechable::Container::Model::operator==(Model const& other) const
{
    return sizes == other.sizes;
}

ILF_NAMESPACE_END
ILF_WARNING_END
