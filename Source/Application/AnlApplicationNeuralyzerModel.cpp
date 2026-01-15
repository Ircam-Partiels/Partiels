#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::ModelInfo::ModelInfo(juce::File const& file)
: modelFile(file)
{
}

bool Application::Neuralyzer::ModelInfo::operator==(ModelInfo const& rhs) const noexcept
{
    static auto const equals = [](std::optional<float> const& lhsf, std::optional<float> const& rhsf)
    {
        return lhsf.has_value() == rhsf.has_value() && (!lhsf.has_value() || std::abs(lhsf.value() - rhsf.value()) < std::numeric_limits<float>::epsilon());
    };

    return modelFile == rhs.modelFile &&
           contextSize == rhs.contextSize &&
           batchSize == rhs.batchSize &&
           equals(minP, rhs.minP) &&
           equals(temperature, rhs.temperature);
}

template <>
void XmlParser::toXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "modelFile", value.modelFile);
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
    value.modelFile = fromXml(*child, "modelFile", defaultValue.modelFile);
    value.contextSize = fromXml(*child, "contextSize", defaultValue.contextSize);
    value.batchSize = fromXml(*child, "batchSize", defaultValue.batchSize);
    value.minP = fromXml(*child, "minP", defaultValue.minP);
    value.temperature = fromXml(*child, "temperature", defaultValue.temperature);
    return value;
}

ANALYSE_FILE_END
