
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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

template<>
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
