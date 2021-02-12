#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

template<>
void XmlParser::toXml<Analyzer::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Analyzer::ColourSet const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "map", value.map);
        toXml(*child, "line", value.line);
        toXml(*child, "background", value.background);
        xml.addChildElement(child.release());
    }
}

template<>
auto XmlParser::fromXml<Analyzer::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Analyzer::ColourSet const& defaultValue)
-> Analyzer::ColourSet
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Analyzer::ColourSet value;
    value.map = fromXml(*child, "map", defaultValue.map);
    value.line = fromXml(*child, "line", defaultValue.line);
    value.background = fromXml(*child, "background", defaultValue.background);
    return value;
}

ANALYSE_FILE_END
