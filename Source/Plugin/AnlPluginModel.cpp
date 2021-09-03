
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
    j.at("identifier").get_to(key.identifier);
    j.at("feature").get_to(key.feature);
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
    j.at("identifier").get_to(output.identifier);
    j.at("name").get_to(output.name);
    j.at("description").get_to(output.description);
    j.at("unit").get_to(output.unit);
    j.at("hasFixedBinCount").get_to(output.hasFixedBinCount);
    j.at("binCount").get_to(output.binCount);
    j.at("binNames").get_to(output.binNames);
    j.at("hasKnownExtents").get_to(output.hasKnownExtents);
    j.at("minValue").get_to(output.minValue);
    j.at("maxValue").get_to(output.maxValue);
    j.at("isQuantized").get_to(output.isQuantized);
    j.at("quantizeStep").get_to(output.quantizeStep);
    j.at("sampleType").get_to(output.sampleType);
    j.at("sampleRate").get_to(output.sampleRate);
    j.at("hasDuration").get_to(output.hasDuration);
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
    j.at("identifier").get_to(parameter.identifier);
    j.at("name").get_to(parameter.name);
    j.at("description").get_to(parameter.description);
    j.at("unit").get_to(parameter.unit);
    j.at("minValue").get_to(parameter.minValue);
    j.at("maxValue").get_to(parameter.maxValue);
    j.at("defaultValue").get_to(parameter.defaultValue);
    j.at("isQuantized").get_to(parameter.isQuantized);
    j.at("quantizeStep").get_to(parameter.quantizeStep);
    j.at("valueNames").get_to(parameter.valueNames);
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
    j.at("blockSize").get_to(state.blockSize);
    j.at("stepSize").get_to(state.stepSize);
    j.at("windowType").get_to(state.windowType);
    j.at("parameters").get_to(state.parameters);
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
    j["programs"] = description.programs;
}

void Plugin::from_json(nlohmann::json const& j, Description& description)
{
    j.at("name").get_to(description.name);
    j.at("inputDomain").get_to(description.inputDomain);
    j.at("maker").get_to(description.maker);
    j.at("version").get_to(description.version);
    j.at("category").get_to(description.category);
    j.at("details").get_to(description.details);
    j.at("defaultState").get_to(description.defaultState);
    j.at("parameters").get_to(description.parameters);
    j.at("output").get_to(description.output);
    j.at("programs").get_to(description.programs);
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
        toXml(*child, "binNames", value.binNames);
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

ANALYSE_FILE_END
