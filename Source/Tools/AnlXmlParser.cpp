#include "AnlXmlParser.h"

ANALYSE_FILE_BEGIN

template<>
void XmlParser::toXml<juce::Range<double>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<double> const& value)
{
    xml.setAttribute(attributeName+"::start", value.getStart());
    xml.setAttribute(attributeName+"::end", value.getEnd());
}

template<>
void XmlParser::toXml<juce::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::File const& value)
{
    xml.setAttribute(attributeName, value.getFullPathName());
}

template<>
auto XmlParser::fromXml<juce::Range<double>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<double> const& defaultValue)
-> juce::Range<double>
{
    anlWeakAssert(xml.hasAttribute(attributeName+"::start"));
    anlWeakAssert(xml.hasAttribute(attributeName+"::end"));
    return {xml.getDoubleAttribute(attributeName+"::start", defaultValue.getStart()),
            xml.getDoubleAttribute(attributeName+"::end", defaultValue.getEnd())};
}


template<>
auto XmlParser::fromXml<juce::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::File const& defaultValue)
-> juce::File
{
    return {xml.getStringAttribute(attributeName, defaultValue.getFullPathName())};
}

class XmlParserUnitTest
: public juce::UnitTest
{
public:
    
    XmlParserUnitTest() : juce::UnitTest("XmlParser", "XmlParser") {}
    
    ~XmlParserUnitTest() override = default;
    
    void runTest() override
    {
        enum Enum
        {
            e0, e1, e2
        };
        
        beginTest("int double float String enum");
        {
            juce::XmlElement xml("Test");
            XmlParser::toXml(xml, "int", 1);
        }
        
        beginTest("unique_ptr");
        {
        }
        
        beginTest("vector map");
        {
            juce::XmlElement xml("Test");
            XmlParser::toXml(xml, "int", std::vector<int>{0, 1, 2});
            XmlParser::toXml(xml, "int", std::map<std::string, int>{{"v0", 0}, {"v1", 1}, {"v2", 2}});
        }
        
        beginTest("file");
        {
        }
        
        beginTest("range");
        {
        }
    }
};

static XmlParserUnitTest xmlParserUnitTest;

ANALYSE_FILE_END
