#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

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
