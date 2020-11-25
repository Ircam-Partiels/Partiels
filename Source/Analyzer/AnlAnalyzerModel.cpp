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
        child->setAttribute("hasTimestamp", value.hasTimestamp);
        if(value.hasTimestamp)
        {
            child->setAttribute("timestamp::sec", value.timestamp.sec);
            child->setAttribute("timestamp::nsec", value.timestamp.nsec);
        }
        if(value.values.size() == 1)
        {
            child->setAttribute("value", static_cast<double>(value.values[0]));
        }
        else if(value.values.size() > 1)
        {
//            juce::StringArray array;
//            array.ensureStorageAllocated(static_cast<int>(value.values.size()));
//            for(auto const& v : value.values)
//            {
//                array.add(juce::String(v));
//            }
//            child->setAttribute("values", array.joinIntoString(","));
        }
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
    value.hasDuration = child->getBoolAttribute("hasTimestamp", defaultValue.hasDuration);
    if(value.hasDuration)
    {
        anlWeakAssert(child->hasAttribute("timestamp::sec") && child->hasAttribute("timestamp::nsec"));
        if(child->hasAttribute("timestamp::sec") && child->hasAttribute("timestamp::nsec"))
        {
            value.timestamp.sec = child->getIntAttribute("timestamp::sec");
            value.timestamp.nsec = child->getIntAttribute("timestamp::nsec");
        }
        else
        {
            value.hasDuration = false;
        }
    }
    if(child->hasAttribute("value"))
    {
        value.values = {static_cast<float>(child->getDoubleAttribute("value"))};
    }
    else if(child->hasAttribute("values"))
    {
        juce::StringArray array;
        auto const size = array.addTokens(child->getStringAttribute("values"), ",", "");
        value.values.reserve(static_cast<size_t>(size));
        for(auto const& str : array)
        {
            value.values.push_back(str.getFloatValue());
        }
    }
    return value;
}

ANALYSE_FILE_END
