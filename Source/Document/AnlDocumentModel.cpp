#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

Document::Model Document::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Anl::Document::Model"));
    if(!xml.hasTagName("Anl::Document::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("file"));
    anlWeakAssert(xml.hasAttribute("isLooping"));
    
    defaultModel.file = Tools::StringParser::fromXml(xml, "file", defaultModel.file);
    defaultModel.isLooping = xml.getBoolAttribute("isLooping", defaultModel.isLooping);
    auto it = defaultModel.analyzers.begin();
    for(auto child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        using submodel_t = decltype(defaultModel.analyzers)::value_type;
        if(it != defaultModel.analyzers.end())
        {
            *it = submodel_t::fromXml(*child, *it);
        }
        else
        {
            it = defaultModel.analyzers.insert(it, submodel_t::fromXml(*child));
        }
        ++it;
    }
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> Document::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Anl::Document::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("file", Tools::StringParser::toString(file));
    for(auto const& analyzer : analyzers)
    {
        auto child = analyzer.toXml();
        anlWeakAssert(child != nullptr);
        if(child != nullptr)
        {
            xml->addChildElement(child.release());
        }
    }
    xml->setAttribute("isLooping", isLooping);
    return xml;
}

std::set<Document::Model::Attribute> Document::Model::getAttributeTypes()
{
    return {Attribute::file, Attribute::analyzers, Attribute::isLooping};
}

void Document::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    MODEL_ACCESSOR_COMPARE_AND_SET(file, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(analyzers, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(isLooping, attributes)
    notifyListener(attributes, {}, notification);
}

ANALYSE_FILE_END
