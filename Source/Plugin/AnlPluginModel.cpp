
#include "AnlPluginModel.h"

namespace Ive
{
    void to_json(nlohmann::json& j, PluginExtension::OutputExtraDescriptor const& outputExtra);
    void from_json(nlohmann::json const& j, PluginExtension::OutputExtraDescriptor& outputExtra);
} // namespace Ive

ANALYSE_FILE_BEGIN

bool Plugin::Output::operator==(Output const& rhs) const noexcept
{
    static auto const isEmpty = [](std::string const& str)
    {
        return str.empty();
    };
    return identifier == rhs.identifier &&
           name == rhs.name &&
           description == rhs.description &&
           unit == rhs.unit &&
           hasFixedBinCount == rhs.hasFixedBinCount &&
           (!hasFixedBinCount || binCount == rhs.binCount) &&
           (binNames == rhs.binNames || (std::all_of(binNames.cbegin(), binNames.cend(), isEmpty) && std::all_of(rhs.binNames.cbegin(), rhs.binNames.cend(), isEmpty))) &&
           hasKnownExtents == rhs.hasKnownExtents &&
           (!hasKnownExtents || std::abs(minValue - rhs.minValue) < std::numeric_limits<float>::epsilon()) &&
           (!hasKnownExtents || std::abs(maxValue - rhs.maxValue) < std::numeric_limits<float>::epsilon()) &&
           isQuantized == rhs.isQuantized &&
           (!isQuantized || std::abs(quantizeStep - rhs.quantizeStep) < std::numeric_limits<float>::epsilon()) &&
           sampleType == rhs.sampleType &&
           (sampleType == SampleType::OneSamplePerStep || std::abs(sampleRate - rhs.sampleRate) < std::numeric_limits<float>::epsilon()) &&
           hasDuration == rhs.hasDuration;
}

bool Plugin::Parameter::operator==(Parameter const& rhs) const noexcept
{
    return identifier == rhs.identifier &&
           name == rhs.name &&
           description == rhs.description &&
           unit == rhs.unit &&
           std::abs(minValue - rhs.minValue) < std::numeric_limits<float>::epsilon() &&
           std::abs(maxValue - rhs.maxValue) < std::numeric_limits<float>::epsilon() &&
           std::abs(defaultValue - rhs.defaultValue) < std::numeric_limits<float>::epsilon() &&
           isQuantized == rhs.isQuantized &&
           (!isQuantized || std::abs(quantizeStep - rhs.quantizeStep) < std::numeric_limits<float>::epsilon()) &&
           valueNames == rhs.valueNames;
}

void Plugin::to_json(nlohmann::json& j, Key const& key)
{
    j["identifier"] = key.identifier;
    j["feature"] = key.feature;
}

void Plugin::from_json(nlohmann::json const& j, Key& key)
{
    key.identifier = j.value("identifier", key.identifier);
    key.feature = j.value("feature", key.feature);
}

void Plugin::to_json(nlohmann::json& j, Output const& output)
{
    j["identifier"] = output.identifier;
    j["name"] = output.name;
    j["description"] = output.description;
    j["unit"] = output.unit;
    j["hasFixedBinCount"] = output.hasFixedBinCount;
    j["binCount"] = output.binCount;
    j["binNames"] = output.binNames;
    j["hasKnownExtents"] = output.hasKnownExtents;
    j["minValue"] = output.minValue;
    j["maxValue"] = output.maxValue;
    j["isQuantized"] = output.isQuantized;
    j["quantizeStep"] = output.quantizeStep;
    j["sampleType"] = output.sampleType;
    j["sampleRate"] = output.sampleRate;
    j["hasDuration"] = output.hasDuration;
}

void Plugin::from_json(nlohmann::json const& j, Output& output)
{
    output.identifier = j.value("identifier", output.identifier);
    output.name = j.value("name", output.name);
    output.description = j.value("description", output.description);
    output.unit = j.value("unit", output.unit);
    output.hasFixedBinCount = j.value("hasFixedBinCount", output.hasFixedBinCount);
    output.binCount = j.value("binCount", output.binCount);
    output.binNames = j.value("binNames", output.binNames);
    output.hasKnownExtents = j.value("hasKnownExtents", output.hasKnownExtents);
    output.minValue = j.value("minValue", output.minValue);
    output.maxValue = j.value("maxValue", output.maxValue);
    output.isQuantized = j.value("isQuantized", output.isQuantized);
    output.quantizeStep = j.value("quantizeStep", output.quantizeStep);
    output.sampleType = j.value("sampleType", output.sampleType);
    output.sampleRate = j.value("sampleRate", output.sampleRate);
    output.hasDuration = j.value("hasDuration", output.hasDuration);
}

void Plugin::to_json(nlohmann::json& j, OutputExtra const& outputExtra)
{
    j["identifier"] = outputExtra.identifier;
    j["name"] = outputExtra.name;
    j["description"] = outputExtra.description;
    j["unit"] = outputExtra.unit;
    j["hasKnownExtents"] = outputExtra.hasKnownExtents;
    j["minValue"] = outputExtra.minValue;
    j["maxValue"] = outputExtra.maxValue;
    j["isQuantized"] = outputExtra.isQuantized;
    j["quantizeStep"] = outputExtra.quantizeStep;
}

void Plugin::from_json(nlohmann::json const& j, OutputExtra& outputExtra)
{
    outputExtra.identifier = j.value("identifier", outputExtra.identifier);
    outputExtra.name = j.value("name", outputExtra.name);
    outputExtra.description = j.value("description", outputExtra.description);
    outputExtra.unit = j.value("unit", outputExtra.unit);
    outputExtra.hasKnownExtents = j.value("hasKnownExtents", outputExtra.hasKnownExtents);
    outputExtra.minValue = j.value("minValue", outputExtra.minValue);
    outputExtra.maxValue = j.value("maxValue", outputExtra.maxValue);
    outputExtra.isQuantized = j.value("isQuantized", outputExtra.isQuantized);
    outputExtra.quantizeStep = j.value("quantizeStep", outputExtra.quantizeStep);
}

void Plugin::to_json(nlohmann::json& j, Parameter const& parameter)
{
    j["identifier"] = parameter.identifier;
    j["name"] = parameter.name;
    j["description"] = parameter.description;
    j["unit"] = parameter.unit;
    j["minValue"] = parameter.minValue;
    j["maxValue"] = parameter.maxValue;
    j["defaultValue"] = parameter.defaultValue;
    j["isQuantized"] = parameter.isQuantized;
    j["quantizeStep"] = parameter.quantizeStep;
    j["valueNames"] = parameter.valueNames;
}

void Plugin::from_json(nlohmann::json const& j, Parameter& parameter)
{
    parameter.identifier = j.value("identifier", parameter.identifier);
    parameter.name = j.value("name", parameter.name);
    parameter.description = j.value("description", parameter.description);
    parameter.unit = j.value("unit", parameter.unit);
    parameter.minValue = j.value("minValue", parameter.minValue);
    parameter.maxValue = j.value("maxValue", parameter.maxValue);
    parameter.defaultValue = j.value("defaultValue", parameter.defaultValue);
    parameter.isQuantized = j.value("isQuantized", parameter.isQuantized);
    parameter.quantizeStep = j.value("quantizeStep", parameter.quantizeStep);
    parameter.valueNames = j.value("valueNames", parameter.valueNames);
}

void Plugin::to_json(nlohmann::json& j, State const& state)
{
    j["blockSize"] = state.blockSize;
    j["stepSize"] = state.stepSize;
    j["windowType"] = state.windowType;
    j["parameters"] = state.parameters;
}

void Plugin::from_json(nlohmann::json const& j, State& state)
{
    state.blockSize = j.value("blockSize", state.blockSize);
    state.stepSize = j.value("stepSize", state.stepSize);
    state.windowType = j.value("windowType", state.windowType);
    state.parameters = j.value("parameters", state.parameters);
}

void Plugin::to_json(nlohmann::json& j, Description const& description)
{
    j["name"] = description.name;
    j["inputDomain"] = description.inputDomain;
    j["maker"] = description.maker;
    j["version"] = description.version;
    j["category"] = description.category;
    j["details"] = description.details;
    j["copyright"] = description.copyright;
    j["defaultState"] = description.defaultState;
    j["parameters"] = description.parameters;
    j["output"] = description.output;
    j["extraOutputs"] = description.extraOutputs;
    j["inputs"] = description.inputs;
    j["programs"] = description.programs;
}

void Plugin::from_json(nlohmann::json const& j, Description& description)
{
    description.name = j.value("name", description.name);
    description.inputDomain = j.value("inputDomain", description.inputDomain);
    description.maker = j.value("maker", description.maker);
    description.version = j.value("version", description.version);
    description.category = j.value("category", description.category);
    description.details = j.value("details", description.details);
    description.copyright = j.value("copyright", description.copyright);
    description.defaultState = j.value("defaultState", description.defaultState);
    description.parameters = j.value("parameters", description.parameters);
    description.output = j.value("output", description.output);
    description.extraOutputs = j.value("extraOutputs", description.extraOutputs);
    if(j.contains("input"))
    {
        description.inputs.resize(std::max(description.inputs.size(), 1_z));
        description.inputs.front() = j.value("input", description.inputs.front());
    }
    else
    {
        description.inputs = j.value("inputs", description.inputs);
    }
    description.programs = j.value("programs", description.programs);
}

void Plugin::to_json(nlohmann::json& j, WebReference const& info)
{
    j = nlohmann::json{{"identifier", info.identifier.toStdString()},
                       {"name", info.name.toStdString()},
                       {"libraryDescription", info.libraryDescription.toStdString()},
                       {"pluginDescription", info.pluginDescription.toStdString()},
                       {"maker", info.maker.toStdString()},
                       {"websiteUrl", info.websiteUrl.toStdString()},
                       {"version", info.version},
                       {"downloadUrlLinux", info.downloadUrlLinux.toStdString()},
                       {"downloadUrlMacos", info.downloadUrlMacos.toStdString()},
                       {"downloadUrlWindows", info.downloadUrlWindows.toStdString()},
                       {"isCompatible", info.isCompatible}};
}

void Plugin::from_json(nlohmann::json const& j, WebReference& info)
{
    info.identifier = juce::String(j.value("identifier", std::string{}));
    info.name = juce::String(j.value("name", std::string{}));
    info.libraryDescription = juce::String(j.value("libraryDescription", std::string{}));
    info.pluginDescription = juce::String(j.value("pluginDescription", std::string{}));
    info.maker = juce::String(j.value("maker", std::string{}));
    info.websiteUrl = juce::String(j.value("websiteUrl", std::string{}));
    // The version of the plugins is formatted as major.minor.bugfix, only the major part is used
    auto const version = j.contains("version") ? j.at("version") : nlohmann::json{};
    if(version.is_number())
    {
        info.version = version.get<unsigned int>();
    }
    else if(version.is_string())
    {
        info.version = static_cast<unsigned int>(Misc::Version::fromString(juce::String{version.get<std::string>()}).major);
    }
    else
    {
        info.version = 0u;
    }
    info.downloadUrlLinux = juce::String(j.value("downloadUrlLinux", std::string{}));
    info.downloadUrlMacos = juce::String(j.value("downloadUrlMacos", std::string{}));
    info.downloadUrlWindows = juce::String(j.value("downloadUrlWindows", std::string{}));
    info.isCompatible = j.value("isCompatible", true);
}

juce::String Plugin::WebReference::getDownloadUrl() const
{
#if JUCE_LINUX
    return downloadUrlLinux;
#elif JUCE_MAC
    return downloadUrlMacos;
#elif JUCE_WINDOWS
    return downloadUrlWindows;
#else
#error "Unsupported platform: getDownloadUrl is not defined. Please add support for your platform."
#endif
}

template <>
void XmlParser::toXml<Plugin::Key>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Key const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "identifier", value.identifier);
    toXml(*child, "feature", value.feature);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::Key>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Key const& defaultValue)
    -> Plugin::Key
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::Key value;
    value.identifier = fromXml(*child, "identifier", defaultValue.identifier);
    value.feature = fromXml(*child, "feature", defaultValue.feature);
    return value;
}

template <>
void XmlParser::toXml<Plugin::Parameter>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Parameter const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "identifier", value.identifier);
    toXml(*child, "name", value.name);
    toXml(*child, "description", value.description);
    toXml(*child, "unit", value.unit);
    toXml(*child, "minValue", value.minValue);
    toXml(*child, "maxValue", value.maxValue);
    toXml(*child, "defaultValue", value.defaultValue);
    toXml(*child, "isQuantized", value.isQuantized);
    toXml(*child, "quantizeStep", value.quantizeStep);
    toXml(*child, "valueNames", value.valueNames);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::Parameter>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Parameter const& defaultValue)
    -> Plugin::Parameter
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::Parameter value;
    value.identifier = fromXml(*child, "identifier", defaultValue.identifier);
    value.name = fromXml(*child, "name", defaultValue.name);
    value.description = fromXml(*child, "description", defaultValue.description);
    value.unit = fromXml(*child, "unit", defaultValue.unit);
    value.minValue = fromXml(*child, "minValue", defaultValue.minValue);
    value.maxValue = fromXml(*child, "maxValue", defaultValue.maxValue);
    value.defaultValue = fromXml(*child, "defaultValue", defaultValue.defaultValue);
    value.isQuantized = fromXml(*child, "isQuantized", defaultValue.isQuantized);
    value.quantizeStep = fromXml(*child, "quantizeStep", defaultValue.quantizeStep);
    value.valueNames = fromXml(*child, "valueNames", defaultValue.valueNames);
    return value;
}

template <>
void XmlParser::toXml<Plugin::Output>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Output const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "identifier", value.identifier);
    toXml(*child, "name", value.name);
    toXml(*child, "description", value.description);
    toXml(*child, "unit", value.unit);
    toXml(*child, "hasFixedBinCount", value.hasFixedBinCount);
    toXml(*child, "binCount", value.binCount);
    // If the plugins has only empty names, this is ignored
    if(std::any_of(value.binNames.cbegin(), value.binNames.cend(), [](auto const& name)
                   {
                       return !name.empty();
                   }))
    {
        toXml(*child, "binNames", value.binNames);
    }
    toXml(*child, "hasKnownExtents", value.hasKnownExtents);
    toXml(*child, "minValue", value.minValue);
    toXml(*child, "maxValue", value.maxValue);
    toXml(*child, "isQuantized", value.isQuantized);
    toXml(*child, "quantizeStep", value.quantizeStep);
    toXml(*child, "sampleType", value.sampleType);
    toXml(*child, "sampleRate", value.sampleRate);
    toXml(*child, "hasDuration", value.hasDuration);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::Output>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Output const& defaultValue)
    -> Plugin::Output
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::Output value;
    value.identifier = fromXml(*child, "identifier", defaultValue.identifier);
    value.name = fromXml(*child, "name", defaultValue.name);
    value.description = fromXml(*child, "description", defaultValue.description);
    value.unit = fromXml(*child, "unit", defaultValue.unit);
    value.hasFixedBinCount = fromXml(*child, "hasFixedBinCount", defaultValue.hasFixedBinCount);
    value.binCount = fromXml(*child, "binCount", defaultValue.binCount);
    value.binNames = fromXml(*child, "binNames", defaultValue.binNames);
    value.hasKnownExtents = fromXml(*child, "hasKnownExtents", defaultValue.hasKnownExtents);
    value.minValue = fromXml(*child, "minValue", defaultValue.minValue);
    value.maxValue = fromXml(*child, "maxValue", defaultValue.maxValue);
    value.isQuantized = fromXml(*child, "isQuantized", defaultValue.isQuantized);
    value.quantizeStep = fromXml(*child, "quantizeStep", defaultValue.quantizeStep);
    value.sampleType = fromXml(*child, "sampleType", defaultValue.sampleType);
    value.sampleRate = fromXml(*child, "sampleRate", defaultValue.sampleRate);
    value.hasDuration = fromXml(*child, "hasDuration", defaultValue.hasDuration);
    return value;
}

template <>
void XmlParser::toXml<Plugin::OutputExtra>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::OutputExtra const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "identifier", value.identifier);
    toXml(*child, "name", value.name);
    toXml(*child, "description", value.description);
    toXml(*child, "unit", value.unit);
    toXml(*child, "hasKnownExtents", value.hasKnownExtents);
    toXml(*child, "minValue", value.minValue);
    toXml(*child, "maxValue", value.maxValue);
    toXml(*child, "isQuantized", value.isQuantized);
    toXml(*child, "quantizeStep", value.quantizeStep);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::OutputExtra>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::OutputExtra const& defaultValue)
    -> Plugin::OutputExtra
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::OutputExtra value;
    value.identifier = fromXml(*child, "identifier", defaultValue.identifier);
    value.name = fromXml(*child, "name", defaultValue.name);
    value.description = fromXml(*child, "description", defaultValue.description);
    value.unit = fromXml(*child, "unit", defaultValue.unit);
    value.hasKnownExtents = fromXml(*child, "hasKnownExtents", defaultValue.hasKnownExtents);
    value.minValue = fromXml(*child, "minValue", defaultValue.minValue);
    value.maxValue = fromXml(*child, "maxValue", defaultValue.maxValue);
    value.isQuantized = fromXml(*child, "isQuantized", defaultValue.isQuantized);
    value.quantizeStep = fromXml(*child, "quantizeStep", defaultValue.quantizeStep);
    return value;
}

template <>
void XmlParser::toXml<Plugin::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Description const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "name", value.name);
    toXml(*child, "inputDomain", value.inputDomain);
    toXml(*child, "maker", value.maker);
    toXml(*child, "version", value.version);
    toXml(*child, "category", value.category);
    toXml(*child, "details", value.details);
    toXml(*child, "copyright", value.copyright);
    toXml(*child, "defaultState", value.defaultState);
    toXml(*child, "parameters", value.parameters);
    toXml(*child, "output", value.output);
    toXml(*child, "extraOutputs", value.extraOutputs);
    toXml(*child, "inputs", value.inputs);
    toXml(*child, "programs", value.programs);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Description const& defaultValue)
    -> Plugin::Description
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::Description value;
    value.name = fromXml(*child, "name", defaultValue.name);
    value.inputDomain = fromXml(*child, "inputDomain", defaultValue.inputDomain);
    value.maker = fromXml(*child, "maker", defaultValue.maker);
    value.version = fromXml(*child, "version", defaultValue.version);
    value.category = fromXml(*child, "category", defaultValue.category);
    value.details = fromXml(*child, "details", defaultValue.details);
    if(child->getChildByName("copyright") == nullptr)
    {
        value.copyright = defaultValue.copyright;
    }
    else
    {
        value.copyright = fromXml(*child, "copyright", defaultValue.copyright);
    }
    value.defaultState = fromXml(*child, "defaultState", defaultValue.defaultState);
    value.parameters = fromXml(*child, "parameters", defaultValue.parameters);
    value.output = fromXml(*child, "output", defaultValue.output);
    value.extraOutputs = fromXml(*child, "extraOutputs", defaultValue.extraOutputs);
    if(child->getChildByName("inputs") == nullptr)
    {
        value.inputs = defaultValue.inputs;
        if(child->getChildByName("input") != nullptr)
        {
            value.inputs.resize(std::max(value.inputs.size(), 1_z));
            value.inputs.front() = fromXml(*child, "input", value.inputs.front());
        }
    }
    else
    {
        value.inputs = fromXml(*child, "inputs", defaultValue.inputs);
    }
    value.programs = fromXml(*child, "programs", defaultValue.programs);
    return value;
}

template <>
void XmlParser::toXml<Plugin::State>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::State const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    toXml(*child, "blockSize", value.blockSize);
    toXml(*child, "stepSize", value.stepSize);
    toXml(*child, "windowType", value.windowType);
    toXml(*child, "parameters", value.parameters);
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<Plugin::State>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::State const& defaultValue)
    -> Plugin::State
{
    auto const* child = xml.getChildByName(attributeName);
    MiscWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Plugin::State value;
    value.blockSize = fromXml(*child, "blockSize", defaultValue.blockSize);
    value.stepSize = fromXml(*child, "stepSize", defaultValue.stepSize);
    value.windowType = fromXml(*child, "windowType", defaultValue.windowType);
    value.parameters = fromXml(*child, "parameters", defaultValue.parameters);
    return value;
}

template <>
void XmlParser::toXml<Plugin::WebReference>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::WebReference const& value)
{
    auto element = std::make_unique<juce::XmlElement>(attributeName);
    element->setAttribute("identifier", value.identifier);
    element->setAttribute("name", value.name);
    element->setAttribute("libraryDescription", value.libraryDescription);
    element->setAttribute("pluginDescription", value.pluginDescription);
    element->setAttribute("maker", value.maker);
    element->setAttribute("websiteUrl", value.websiteUrl);
    element->setAttribute("version", static_cast<int>(value.version));
    element->setAttribute("downloadUrlLinux", value.downloadUrlLinux);
    element->setAttribute("downloadUrlMacos", value.downloadUrlMacos);
    element->setAttribute("downloadUrlWindows", value.downloadUrlWindows);
    element->setAttribute("isCompatible", value.isCompatible);
    xml.addChildElement(element.release());
}

template <>
auto XmlParser::fromXml<Plugin::WebReference>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::WebReference const& defaultValue)
    -> Plugin::WebReference
{
    auto const* element = xml.getChildByName(attributeName);
    if(element == nullptr)
    {
        return defaultValue;
    }
    Plugin::WebReference info;
    info.identifier = element->getStringAttribute("identifier", defaultValue.identifier);
    info.name = element->getStringAttribute("name", defaultValue.name);
    info.libraryDescription = element->getStringAttribute("libraryDescription", defaultValue.libraryDescription);
    info.pluginDescription = element->getStringAttribute("pluginDescription", defaultValue.pluginDescription);
    info.maker = element->getStringAttribute("maker", defaultValue.maker);
    info.websiteUrl = element->getStringAttribute("websiteUrl", defaultValue.websiteUrl);
    info.version = static_cast<unsigned int>(element->getIntAttribute("version", static_cast<int>(defaultValue.version)));
    info.downloadUrlLinux = element->getStringAttribute("downloadUrlLinux", defaultValue.downloadUrlLinux);
    info.downloadUrlMacos = element->getStringAttribute("downloadUrlMacos", defaultValue.downloadUrlMacos);
    info.downloadUrlWindows = element->getStringAttribute("downloadUrlWindows", defaultValue.downloadUrlWindows);
    info.isCompatible = element->getBoolAttribute("isCompatible", defaultValue.isCompatible);
    return info;
}

Plugin::Description Plugin::loadDescription(Ive::PluginWrapper& plugin, Plugin::Key const& key)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {};
    }

    auto const outputs = plugin.getOutputDescriptors();
    auto const outputIt = std::find_if(outputs.cbegin(), outputs.cend(), [&](auto const& output)
                                       {
                                           return output.identifier == key.feature;
                                       });
    if(outputIt == outputs.cend())
    {
        return {};
    }
    Plugin::Description description;
    description.name = plugin.getName();
    description.inputDomain = Vamp::Plugin::InputDomain::TimeDomain;
    if(auto* inputDomainAdapter = plugin.getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
    {
        description.inputDomain = Vamp::Plugin::InputDomain::FrequencyDomain;
        description.defaultState.windowType = inputDomainAdapter->getWindowType();
    }

    description.maker = plugin.getMaker();
    description.version = static_cast<unsigned int>(plugin.getPluginVersion());
    auto const categories = pluginLoader->getPluginCategory(key.identifier);
    description.category = categories.empty() ? "" : categories.front();
    description.details = plugin.getDescription();
    description.copyright = juce::String(plugin.getCopyright());
    auto const parameters = plugin.getParameterDescriptors();
    description.parameters.insert(description.parameters.cbegin(), parameters.cbegin(), parameters.cend());

    auto initializeState = [&](Plugin::State& state, bool defaultValues)
    {
        state.blockSize = plugin.getPreferredBlockSize();
        state.stepSize = plugin.getPreferredStepSize();
        for(auto const& parameter : parameters)
        {
            state.parameters[parameter.identifier] = defaultValues ? parameter.defaultValue : plugin.getParameter(parameter.identifier);
        }
    };
    initializeState(description.defaultState, true);
    auto const programNames = plugin.getPrograms();
    for(auto const& programName : programNames)
    {
        plugin.selectProgram(programName);
        initializeState(description.programs[programName], false);
    }

    description.output = *outputIt;
    // The Vamp host generates empty bin names if the bin count is not null to ensure backward compatibility.
    // In this case, we clear the bin names to avoid confusion, as they don't provide any useful information.
    if(std::all_of(description.output.binNames.cbegin(), description.output.binNames.cend(), [](auto const& str)
                   {
                       return str.empty();
                   }))
    {
        description.output.binNames.clear();
    }
    auto const outputIndex = static_cast<size_t>(std::distance(outputs.cbegin(), outputIt));
    description.extraOutputs = plugin.getOutputExtraDescriptors(outputIndex);
    auto const inputs = plugin.getInputDescriptors();
    description.inputs.clear();
    description.inputs.reserve(inputs.size());
    for(auto const& input : inputs)
    {
        description.inputs.push_back(input);
    }
    return description;
}

ANALYSE_FILE_END

void Ive::to_json(nlohmann::json& j, Ive::PluginExtension::OutputExtraDescriptor const& outputExtra)
{
    Misc::Plugin::to_json(j, outputExtra);
}

void Ive::from_json(nlohmann::json const& j, Ive::PluginExtension::OutputExtraDescriptor& outputExtra)
{
    Misc::Plugin::from_json(j, outputExtra);
}
