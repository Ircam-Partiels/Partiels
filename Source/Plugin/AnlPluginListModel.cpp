
#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

PluginList::Model PluginList::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Anl::PluginList::Model"));
    if(!xml.hasTagName("Anl::PluginList::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("sortColumn"));
    anlWeakAssert(xml.hasAttribute("sortIsFowards"));
    
    for(auto* child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        anlWeakAssert(child->hasAttribute("key"));
        anlWeakAssert(child->hasAttribute("name"));
        anlWeakAssert(child->hasAttribute("maker"));
        anlWeakAssert(child->hasAttribute("api"));
        anlWeakAssert(child->hasAttribute("details"));
        auto const key = child->getStringAttribute("key");
        if(key.isNotEmpty())
        {
            auto& description = defaultModel.descriptions[key];
            description.name = child->getStringAttribute("name", description.name);
            description.maker = child->getStringAttribute("maker", description.maker);
            description.api = static_cast<unsigned int>(child->getIntAttribute("api", static_cast<int>(description.api)));
            description.details = child->getStringAttribute("details", description.details);
            
            for(auto* subchild = child->getFirstChildElement(); subchild != nullptr; subchild = subchild->getNextElement())
            {
                anlWeakAssert(child->hasAttribute("category"));
                if(child->hasAttribute("category"))
                {
                    description.categories.insert(child->getStringAttribute("category"));
                }
            }
        }
    }
    defaultModel.sortColumn = static_cast<ColumnType>(xml.getIntAttribute("sortColumn", static_cast<int>(defaultModel.sortColumn)));
    defaultModel.sortIsFowards = xml.getBoolAttribute("sortIsFowards", defaultModel.sortIsFowards);
    
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> PluginList::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Anl::PluginList::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }

    for(auto const& description : descriptions)
    {
        auto child = std::make_unique<juce::XmlElement>("Description");
        anlWeakAssert(child != nullptr);
        if(child != nullptr)
        {
            child->setAttribute("key", description.first);
            child->setAttribute("name", description.second.name);
            child->setAttribute("maker", description.second.maker);
            child->setAttribute("api", static_cast<int>(description.second.api));
            child->setAttribute("details", description.second.details);
            for(auto const& category : description.second.categories)
            {
                auto subchild = std::make_unique<juce::XmlElement>("Category");
                anlWeakAssert(subchild != nullptr);
                if(subchild != nullptr)
                {
                    subchild->setAttribute("category", category);
                    child->addChildElement(subchild.release());
                }
            }
            xml->addChildElement(child.release());
        }
    }
    xml->setAttribute("sortColumn", static_cast<int>(sortColumn));
    xml->setAttribute("sortIsFowards", sortIsFowards);
    
    return xml;
}

std::set<PluginList::Model::Attribute> PluginList::Model::getAttributeTypes()
{
    return {Attribute::descriptions, Attribute::sortColumn, Attribute::sortIsFowards};
}

void PluginList::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    MODEL_ACCESSOR_COMPARE_AND_SET(descriptions, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(sortColumn, attributes)
    MODEL_ACCESSOR_COMPARE_AND_SET(sortIsFowards, attributes)
    notifyListener(attributes, {}, notification);
}

ANALYSE_FILE_END
