#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

template<>
void XmlParser::toXml<Analyzer::Result>(juce::XmlElement& xml, juce::Identifier const& attributeName, Analyzer::Result const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        child->setAttribute("label", juce::String(value.label));
        if(value.hasDuration)
        {
            JUCE_COMPILER_WARNING("to do")
            //child->setAttribute("duration", value.duration.toString());
        }
        if(value.hasTimestamp)
        {
            //child->setAttribute("timestamp", value.timestamp.toString());
        }
        XmlParser::toXml(*child.get(), "values", value.values);
        xml.addChildElement(child.release());
    }
}

template<>
auto XmlParser::fromXml<Analyzer::Result>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Analyzer::Result const& defaultValue)
-> Analyzer::Result
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    anlWeakAssert(child->hasAttribute("label"));
    Analyzer::Result value;
    JUCE_COMPILER_WARNING("to do")
    value.label = child->getStringAttribute("label", defaultValue.label).toStdString();
    value.values = XmlParser::fromXml(*child, "values", defaultValue.values);
    return value;
}

ANALYSE_FILE_END
