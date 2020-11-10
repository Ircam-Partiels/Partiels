
#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

bool PluginList::Description::operator==(Description const& rhd) const
{
    return name == rhd.name && maker == rhd.maker && api == rhd.api && categories == rhd.categories && details == rhd.details;
}

bool PluginList::Description::operator!=(Description const& rhd) const
{
    return !(*this == rhd);
}

PluginList::Description::Description(juce::String const&)
{
    
}

PluginList::Description::operator juce::String() const
{
    return "";
}

//PluginList::Model PluginList::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
//{
//    anlWeakAssert(xml.hasTagName("Anl::PluginList::Model"));
//    if(!xml.hasTagName("Anl::PluginList::Model"))
//    {
//        return {};
//    }
//    
//    anlWeakAssert(xml.hasAttribute("sortColumn"));
//    anlWeakAssert(xml.hasAttribute("sortIsFowards"));
//    
//    auto const childs = Tools::XmlUtils::getChilds(xml, "Description");
//    for(auto const& child : childs)
//    {
//        anlWeakAssert(child.get().hasAttribute("key"));
//        anlWeakAssert(child.get().hasAttribute("name"));
//        anlWeakAssert(child.get().hasAttribute("maker"));
//        anlWeakAssert(child.get().hasAttribute("api"));
//        anlWeakAssert(child.get().hasAttribute("details"));
//        auto const key = child.get().getStringAttribute("key");
//        if(key.isNotEmpty())
//        {
//            auto& description = defaultModel.descriptions[key];
//            description.name = child.get().getStringAttribute("name", description.name);
//            description.maker = child.get().getStringAttribute("maker", description.maker);
//            description.api = static_cast<unsigned int>(child.get().getIntAttribute("api", static_cast<int>(description.api)));
//            description.details = child.get().getStringAttribute("details", description.details);
//            
//            auto const subchilds = Tools::XmlUtils::getChilds(child.get(), "Category");
//            for(auto const& subchild : subchilds)
//            {
//                anlWeakAssert(subchild.get().hasAttribute("category"));
//                if(subchild.get().hasAttribute("category"))
//                {
//                    description.categories.insert(subchild.get().getStringAttribute("category"));
//                }
//            }
//        }
//    }
//    defaultModel.sortColumn = static_cast<ColumnType>(xml.getIntAttribute("sortColumn", static_cast<int>(defaultModel.sortColumn)));
//    defaultModel.sortIsFowards = xml.getBoolAttribute("sortIsFowards", defaultModel.sortIsFowards);
//    
//    return defaultModel;
//}
//
//std::unique_ptr<juce::XmlElement> PluginList::Model::toXml() const
//{
//    auto xml = std::make_unique<juce::XmlElement>("Anl::PluginList::Model");
//    if(xml == nullptr)
//    {
//        return nullptr;
//    }
//
//    for(auto const& description : descriptions)
//    {
//        auto child = std::make_unique<juce::XmlElement>("Description");
//        anlWeakAssert(child != nullptr);
//        if(child != nullptr)
//        {
//            child->setAttribute("key", description.first);
//            child->setAttribute("name", description.second.name);
//            child->setAttribute("maker", description.second.maker);
//            child->setAttribute("api", static_cast<int>(description.second.api));
//            child->setAttribute("details", description.second.details);
//            for(auto const& category : description.second.categories)
//            {
//                auto subchild = std::make_unique<juce::XmlElement>("Category");
//                anlWeakAssert(subchild != nullptr);
//                if(subchild != nullptr)
//                {
//                    subchild->setAttribute("category", category);
//                    child->addChildElement(subchild.release());
//                }
//            }
//            xml->addChildElement(child.release());
//        }
//    }
//    xml->setAttribute("sortColumn", static_cast<int>(sortColumn));
//    xml->setAttribute("sortIsFowards", sortIsFowards);
//    
//    return xml;
//}

ANALYSE_FILE_END
