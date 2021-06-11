#include "AnlXmlParser.h"

ANALYSE_FILE_BEGIN

template <>
void XmlParser::toXml<juce::Range<double>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<double> const& value)
{
    xml.setAttribute(attributeName + "_start", value.getStart());
    xml.setAttribute(attributeName + "_end", value.getEnd());
}

template <>
auto XmlParser::fromXml<juce::Range<double>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<double> const& defaultValue)
-> juce::Range<double>
{
    anlWeakAssert(xml.hasAttribute(attributeName + "_start") || xml.hasAttribute(attributeName + "::start"));
    anlWeakAssert(xml.hasAttribute(attributeName + "_end") || xml.hasAttribute(attributeName + "::end"));
    auto const start = xml.hasAttribute(attributeName + "_start") ? xml.getDoubleAttribute(attributeName + "_start") : xml.getDoubleAttribute(attributeName + "::start", defaultValue.getStart());
    auto const end = xml.hasAttribute(attributeName + "_end") ? xml.getDoubleAttribute(attributeName + "_end") : xml.getDoubleAttribute(attributeName + "::end", defaultValue.getEnd());
    return {start, end};
}

template <>
void XmlParser::toXml<juce::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::File const& value)
{
    xml.setAttribute(attributeName, value.getFullPathName());
}

template <>
auto XmlParser::fromXml<juce::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::File const& defaultValue)
-> juce::File
{
    return {xml.getStringAttribute(attributeName, defaultValue.getFullPathName())};
}

template <>
void XmlParser::toXml<juce::Point<int>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Point<int> const& value)
{
    xml.setAttribute(attributeName + "_x", value.getX());
    xml.setAttribute(attributeName + "_y", value.getY());
}

template <>
auto XmlParser::fromXml<juce::Point<int>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Point<int> const& defaultValue)
-> juce::Point<int>
{
    anlWeakAssert(xml.hasAttribute(attributeName + "_x") || xml.hasAttribute(attributeName + "::x"));
    anlWeakAssert(xml.hasAttribute(attributeName + "_y") || xml.hasAttribute(attributeName + "::y"));
    auto const x = xml.hasAttribute(attributeName + "_x") ? xml.getIntAttribute(attributeName + "_x") : xml.getIntAttribute(attributeName + "::x", defaultValue.getX());
    auto const y = xml.hasAttribute(attributeName + "_y") ? xml.getIntAttribute(attributeName + "_y") : xml.getIntAttribute(attributeName + "::y", defaultValue.getY());
    return {x, y};
}

class XmlParserUnitTest
: public juce::UnitTest
{
public:
    XmlParserUnitTest()
    : juce::UnitTest("XmlParser", "XmlParser")
    {
    }

    ~XmlParserUnitTest() override = default;

    void runTest() override
    {
        // clang-format off
        enum Enum
        {
              e0
            , e1
            , e2
        };
        // clang-format on

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
