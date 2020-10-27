#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

Analyzer::Model Analyzer::Model::fromXml(juce::XmlElement const& xml, Model defaultModel)
{
    anlWeakAssert(xml.hasTagName("Anl::Analyzer::Model"));
    if(!xml.hasTagName("Anl::Analyzer::Model"))
    {
        return {};
    }
    
    anlWeakAssert(xml.hasAttribute("key"));
    anlWeakAssert(xml.hasAttribute("programName"));
    anlWeakAssert(xml.hasAttribute("resultFile"));
    anlWeakAssert(xml.hasAttribute("projectFile"));
    
    defaultModel.key = xml.getStringAttribute("key", defaultModel.key);
    defaultModel.programName = xml.getStringAttribute("programName", defaultModel.programName);
    defaultModel.resultFile = xml.getStringAttribute("file", defaultModel.resultFile.getFullPathName());
    defaultModel.projectFile = xml.getStringAttribute("project", defaultModel.projectFile.getFullPathName());
    auto const childs = Tools::XmlUtils::getChilds(xml, "Parameter");
    for(auto const& child : childs)
    {
        anlWeakAssert(child.get().hasAttribute("name"));
        anlWeakAssert(child.get().hasAttribute("value"));
        auto const key = child.get().getStringAttribute("name");
        if(key.isNotEmpty())
        {
            auto& parameter = defaultModel.parameters[key];
            parameter = child.get().getDoubleAttribute("value", parameter);
        }
    }
    
    return defaultModel;
}

std::unique_ptr<juce::XmlElement> Analyzer::Model::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("As::Analyzer::Model");
    if(xml == nullptr)
    {
        return nullptr;
    }
    
    xml->setAttribute("key", key);
    xml->setAttribute("programName", programName);
    xml->setAttribute("resultFile", resultFile.getFullPathName());
    xml->setAttribute("projectFile", projectFile.getFullPathName());
    for(auto const& parameter : parameters)
    {
        auto child = std::make_unique<juce::XmlElement>("Parameter");
        anlWeakAssert(child != nullptr);
        if(child != nullptr)
        {
            child->setAttribute("name", parameter.first);
            child->setAttribute("value", parameter.second);
            xml->addChildElement(child.release());
        }
    }
    return xml;
}

std::set<Analyzer::Model::Attribute> Analyzer::Model::getAttributeTypes()
{
    return {Attribute::key, Attribute::parameters, Attribute::programName, Attribute::resultFile, Attribute::projectFile};
}

void Analyzer::Accessor::fromModel(Model const& model, juce::NotificationType const notification)
{
    using Attribute = Model::Attribute;
    std::set<Attribute> attributes;
    MODEL_ACCESSOR_COMPARE_AND_SET(key, attributes);
    MODEL_ACCESSOR_COMPARE_AND_SET(parameters, attributes);
    MODEL_ACCESSOR_COMPARE_AND_SET(programName, attributes);
    MODEL_ACCESSOR_COMPARE_AND_SET(resultFile, attributes);
    MODEL_ACCESSOR_COMPARE_AND_SET(projectFile, attributes);
    notifyListener(attributes, {}, notification);
}

ANALYSE_FILE_END
