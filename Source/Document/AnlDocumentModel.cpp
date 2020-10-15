#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

Document::Model Document::Model::fromXml(juce::XmlElement const& xml)
{
    anlWeakAssert(xml.hasTagName("Anl::Document::Model"));
    if(!xml.hasTagName("Anl::Document::Model"))
    {
        return {};
    }
    
    Model model;
    return model;
}

std::unique_ptr<juce::XmlElement> Document::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("As::Document::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    return xml;
}

ANALYSE_FILE_END
