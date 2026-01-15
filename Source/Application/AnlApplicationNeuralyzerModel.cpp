#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

void Application::Neuralyzer::to_json(nlohmann::json& j, ModelInfo const& modelInfo)
{
    j["model"] = modelInfo.model;
    j["tplt"] = modelInfo.tplt;
    j["contextSize"] = modelInfo.contextSize;
    j["batchSize"] = modelInfo.batchSize;
}

void Application::Neuralyzer::from_json(nlohmann::json const& j, ModelInfo& modelInfo)
{
    modelInfo.model = j.value("model", modelInfo.model);
    modelInfo.tplt = j.value("tplt", modelInfo.tplt);
    modelInfo.contextSize = j.value("contextSize", modelInfo.contextSize);
    modelInfo.batchSize = j.value("batchSize", modelInfo.batchSize);
}

template <>
void XmlParser::toXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "model", value.model);
    toXml(*child, "tplt", value.tplt);
    toXml(*child, "contextSize", value.contextSize);
    toXml(*child, "batchSize", value.batchSize);
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
    return value;
}

ANALYSE_FILE_END
