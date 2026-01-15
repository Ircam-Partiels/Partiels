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
           equals(temperature, rhs.temperature) &&
           equals(topP, rhs.topP) &&
           topK == rhs.topK &&
           equals(presencePenalty, rhs.presencePenalty) &&
           equals(repetitionPenalty, rhs.repetitionPenalty);
}

juce::File Application::Neuralyzer::resolveNeuralyzerDirectory(juce::File const& root)
{
#if JUCE_MAC
    return root.getChildFile("Application Support").getChildFile("Ircam").getChildFile("Partiels").getChildFile("Neuralyzer");
#else
    return root.getChildFile("Ircam").getChildFile("Partiels").getChildFile("Neuralyzer");
#endif
}

juce::File Application::Neuralyzer::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return Application::Neuralyzer::resolveNeuralyzerDirectory(root).getChildFile("Models");
}

std::pair<juce::File, juce::File> Application::Neuralyzer::getNeuralyzerSessionFile(juce::File const& documentFile)
{
    auto const root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto const directory = Application::Neuralyzer::resolveNeuralyzerDirectory(root).getChildFile("Sessions");
    auto const hash = juce::String::toHexString(static_cast<juce::int64>(documentFile.getFullPathName().hashCode64()));
    return std::make_pair(directory.getChildFile(hash + ".session"), directory.getChildFile(hash + ".messages.json"));
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
    toXml(*child, "topP", value.topP);
    toXml(*child, "topK", value.topK);
    toXml(*child, "presencePenalty", value.presencePenalty);
    toXml(*child, "repetitionPenalty", value.repetitionPenalty);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& defaultValue)
    -> Application::Neuralyzer::ModelInfo
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
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
    value.topP = fromXml(*child, "topP", defaultValue.topP);
    value.topK = fromXml(*child, "topK", defaultValue.topK);
    value.presencePenalty = fromXml(*child, "presencePenalty", defaultValue.presencePenalty);
    value.repetitionPenalty = fromXml(*child, "repetitionPenalty", defaultValue.repetitionPenalty);
    return value;
}

ANALYSE_FILE_END
