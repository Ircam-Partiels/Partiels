#include "AnlApplicationNeuralyzerModel.h"
#include <llama-cpp.h>

ANALYSE_FILE_BEGIN

static auto constexpr magicContextRatio = static_cast<int>(200000 / 36864); // 200000 context size for 36GB
int const Application::Neuralyzer::maxContextSize = juce::SystemStats::getMemorySizeInMegabytes() * magicContextRatio;

Application::Neuralyzer::ModelInfo::ModelInfo(juce::File const& file)
: modelFile(file)
{
}

Application::Neuralyzer::ModelInfo::ModelInfo(juce::URL const& url)
: modelUrl(url)
{
}

bool Application::Neuralyzer::ModelInfo::operator==(ModelInfo const& rhs) const noexcept
{
    static auto const equals = [](std::optional<float> const& lhsf, std::optional<float> const& rhsf)
    {
        return lhsf.has_value() == rhsf.has_value() && (!lhsf.has_value() || std::abs(lhsf.value() - rhsf.value()) < std::numeric_limits<float>::epsilon());
    };

    return modelFile == rhs.modelFile &&
           modelUrl == rhs.modelUrl &&
           modelId == rhs.modelId &&
           contextSize == rhs.contextSize &&
           batchSize == rhs.batchSize &&
           equals(minP, rhs.minP) &&
           equals(temperature, rhs.temperature) &&
           equals(topP, rhs.topP) &&
           topK == rhs.topK &&
           equals(presencePenalty, rhs.presencePenalty) &&
           equals(repetitionPenalty, rhs.repetitionPenalty);
}

static std::atomic<bool> sBackendInitialized{false};

void Application::Neuralyzer::initializeEngine()
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
                   {
                       MiscDebug("Application::Neuralyzer::AgentLocal", "Global Initialize...");
                       llama_log_set([](enum ggml_log_level, [[maybe_unused]] char const* text, void*)
                                     {
                                         MiscDebug("Application::Neuralyzer::AgentLocal::Init", juce::String::fromUTF8(text));
                                     },
                                     nullptr);
                       llama_backend_init();
                       sBackendInitialized.store(true);
                       MiscDebug("Application::Neuralyzer::AgentLocal", "Global Initialized");
                   });
}

void Application::Neuralyzer::releaseEngine()
{
    static std::once_flag releaseFlag;
    std::call_once(releaseFlag, []()
                   {
                       if(!sBackendInitialized.load())
                       {
                           return;
                       }
                       MiscDebug("Application::Neuralyzer::AgentLocal", "Global Release...");
                       llama_backend_free();
                       llama_log_set(nullptr, nullptr);
                       MiscDebug("Application::Neuralyzer::AgentLocal", "Global Released");
                   });
}

juce::File Application::Neuralyzer::resolveDirectory(juce::File const& root)
{
#if JUCE_MAC
    return root.getChildFile("Application Support").getChildFile("Ircam").getChildFile("Partiels").getChildFile("Neuralyzer");
#else
    return root.getChildFile("Ircam").getChildFile("Partiels").getChildFile("Neuralyzer");
#endif
}

juce::File Application::Neuralyzer::getSessionFile(juce::File const& documentFile)
{
    auto const root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto const directory = resolveDirectory(root).getChildFile("Sessions");
    auto const hash = juce::String::toHexString(static_cast<juce::int64>(documentFile.getFullPathName().hashCode64()));
    return directory.getChildFile(hash + ".messages.json");
}

template <>
void XmlParser::toXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "modelFile", value.modelFile);
    toXml(*child, "modelUrl", value.modelUrl);
    toXml(*child, "modelId", value.modelId);
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
    value.modelUrl = fromXml(*child, "modelUrl", defaultValue.modelUrl);
    value.modelId = fromXml(*child, "modelId", defaultValue.modelId);
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
