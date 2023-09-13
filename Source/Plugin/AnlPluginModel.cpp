
#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

bool Plugin::Output::operator==(Output const& rhd) const noexcept
{
    return identifier == rhd.identifier &&
           name == rhd.name &&
           description == rhd.description &&
           unit == rhd.unit &&
           hasFixedBinCount == rhd.hasFixedBinCount &&
           (!hasFixedBinCount || binCount == rhd.binCount) &&
           binNames == rhd.binNames &&
           hasKnownExtents == rhd.hasKnownExtents &&
           (!hasKnownExtents || std::abs(minValue - rhd.minValue) < std::numeric_limits<float>::epsilon()) &&
           (!hasKnownExtents || std::abs(maxValue - rhd.maxValue) < std::numeric_limits<float>::epsilon()) &&
           isQuantized == rhd.isQuantized &&
           (!isQuantized || std::abs(quantizeStep - rhd.quantizeStep) < std::numeric_limits<float>::epsilon()) &&
           sampleType == rhd.sampleType &&
           (sampleType == SampleType::OneSamplePerStep || std::abs(sampleRate - rhd.sampleRate) < std::numeric_limits<float>::epsilon()) &&
           hasDuration == rhd.hasDuration;
}

bool Plugin::Parameter::operator==(Parameter const& rhd) const noexcept
{
    return identifier == rhd.identifier &&
           name == rhd.name &&
           description == rhd.description &&
           unit == rhd.unit &&
           std::abs(minValue - rhd.minValue) < std::numeric_limits<float>::epsilon() &&
           std::abs(maxValue - rhd.maxValue) < std::numeric_limits<float>::epsilon() &&
           std::abs(defaultValue - rhd.defaultValue) < std::numeric_limits<float>::epsilon() &&
           isQuantized == rhd.isQuantized &&
           (!isQuantized || std::abs(quantizeStep - rhd.quantizeStep) < std::numeric_limits<float>::epsilon()) &&
           valueNames == rhd.valueNames;
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
    j["defaultState"] = description.defaultState;
    j["parameters"] = description.parameters;
    j["output"] = description.output;
    j["input"] = description.input;
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
    description.defaultState = j.value("defaultState", description.defaultState);
    description.parameters = j.value("parameters", description.parameters);
    description.output = j.value("output", description.output);
    description.input = j.value("input", description.output);
    description.programs = j.value("programs", description.programs);
}

template <>
void XmlParser::toXml<Plugin::Key>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Key const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "identifier", value.identifier);
        toXml(*child, "feature", value.feature);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Plugin::Key>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Key const& defaultValue)
    -> Plugin::Key
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
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
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
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
}

template <>
auto XmlParser::fromXml<Plugin::Parameter>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Parameter const& defaultValue)
    -> Plugin::Parameter
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
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
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
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
}

template <>
auto XmlParser::fromXml<Plugin::Output>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Output const& defaultValue)
    -> Plugin::Output
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
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
void XmlParser::toXml<Plugin::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::Description const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "name", value.name);
        toXml(*child, "inputDomain", value.inputDomain);
        toXml(*child, "maker", value.maker);
        toXml(*child, "version", value.version);
        toXml(*child, "category", value.category);
        toXml(*child, "details", value.details);
        toXml(*child, "defaultState", value.defaultState);
        toXml(*child, "parameters", value.parameters);
        toXml(*child, "output", value.output);
        toXml(*child, "input", value.input);
        toXml(*child, "programs", value.programs);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Plugin::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::Description const& defaultValue)
    -> Plugin::Description
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
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
    value.defaultState = fromXml(*child, "defaultState", defaultValue.defaultState);
    value.parameters = fromXml(*child, "parameters", defaultValue.parameters);
    value.output = fromXml(*child, "output", defaultValue.output);
    value.input = fromXml(*child, "input", defaultValue.input);
    value.programs = fromXml(*child, "programs", defaultValue.programs);
    return value;
}

template <>
void XmlParser::toXml<Plugin::State>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plugin::State const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "blockSize", value.blockSize);
        toXml(*child, "stepSize", value.stepSize);
        toXml(*child, "windowType", value.windowType);
        toXml(*child, "parameters", value.parameters);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Plugin::State>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plugin::State const& defaultValue)
    -> Plugin::State
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
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

Plugin::Description Plugin::loadDescription(Ive::PluginWrapper& plugin, Plugin::Key const& key)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
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
    if(!description.details.isEmpty())
    {
        description.details += "\n";
    }
    description.details += juce::String(plugin.getCopyright());
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
    auto const inputs = plugin.getInputDescriptors();
    auto const inputIt = std::find_if(inputs.cbegin(), inputs.cend(), [&](auto const& input)
                                      {
                                          return input.identifier == key.feature;
                                      });
    if(inputIt != inputs.cend())
    {
        description.input = *inputIt;
    }
    return description;
}

ANALYSE_FILE_END
