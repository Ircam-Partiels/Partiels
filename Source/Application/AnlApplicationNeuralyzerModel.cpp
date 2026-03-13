#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::ModelInfo::ModelInfo(Accessor const& accessor)
: model(accessor.getAttr<AttrType::modelFile>())
, tplt(model.withFileExtension(".jinja").existsAsFile() ? model.withFileExtension(".jinja") : juce::File())
, contextSize(std::make_optional(accessor.getAttr<AttrType::contextSize>()))
, batchSize(std::make_optional(accessor.getAttr<AttrType::batchSize>()))
, minP(std::make_optional(accessor.getAttr<AttrType::minP>()))
, temperature(std::make_optional(accessor.getAttr<AttrType::temperature>()))
{
}

template <>
void XmlParser::toXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "model", value.model);
    toXml(*child, "tplt", value.tplt);
    toXml(*child, "contextSize", value.contextSize);
    toXml(*child, "batchSize", value.batchSize);
    toXml(*child, "minP", value.minP);
    toXml(*child, "temperature", value.temperature);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& defaultValue)
    -> Application::Neuralyzer::ModelInfo
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Application::Neuralyzer::ModelInfo value;
    value.model = fromXml(*child, "model", defaultValue.model);
    value.tplt = fromXml(*child, "tplt", defaultValue.tplt);
    value.contextSize = fromXml(*child, "contextSize", defaultValue.contextSize);
    value.batchSize = fromXml(*child, "batchSize", defaultValue.batchSize);
    value.minP = fromXml(*child, "minP", defaultValue.minP);
    value.temperature = fromXml(*child, "temperature", defaultValue.temperature);
    return value;
}

ANALYSE_FILE_END
