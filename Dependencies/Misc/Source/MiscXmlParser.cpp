#include "MiscXmlParser.h"

MISC_FILE_BEGIN

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
    MiscWeakAssert(xml.hasAttribute(attributeName + "_start") || xml.hasAttribute(attributeName + "::start"));
    MiscWeakAssert(xml.hasAttribute(attributeName + "_end") || xml.hasAttribute(attributeName + "::end"));
    auto const start = xml.hasAttribute(attributeName + "_start") ? xml.getDoubleAttribute(attributeName + "_start") : xml.getDoubleAttribute(attributeName + "::start", defaultValue.getStart());
    auto const end = xml.hasAttribute(attributeName + "_end") ? xml.getDoubleAttribute(attributeName + "_end") : xml.getDoubleAttribute(attributeName + "::end", defaultValue.getEnd());
    return {start, end};
}

template <>
void XmlParser::toXml<juce::Range<int>>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Range<int> const& value)
{
    xml.setAttribute(attributeName + "_start", value.getStart());
    xml.setAttribute(attributeName + "_end", value.getEnd());
}

template <>
auto XmlParser::fromXml<juce::Range<int>>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Range<int> const& defaultValue)
    -> juce::Range<int>
{
    MiscWeakAssert(xml.hasAttribute(attributeName + "_start"));
    MiscWeakAssert(xml.hasAttribute(attributeName + "_end"));
    auto const start = xml.getIntAttribute(attributeName + "_start", defaultValue.getStart());
    auto const end = xml.getIntAttribute(attributeName + "_end", defaultValue.getEnd());
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
void XmlParser::toXml<juce::Font>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::Font const& value)
{
    xml.setAttribute(attributeName, value.toString());
}

template <>
auto XmlParser::fromXml<juce::Font>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::Font const& defaultValue)
    -> juce::Font
{
    return juce::Font::fromString(xml.getStringAttribute(attributeName, defaultValue.toString()));
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
    MiscWeakAssert(xml.hasAttribute(attributeName + "_x") || xml.hasAttribute(attributeName + "::x"));
    MiscWeakAssert(xml.hasAttribute(attributeName + "_y") || xml.hasAttribute(attributeName + "::y"));
    auto const x = xml.hasAttribute(attributeName + "_x") ? xml.getIntAttribute(attributeName + "_x") : xml.getIntAttribute(attributeName + "::x", defaultValue.getX());
    auto const y = xml.hasAttribute(attributeName + "_y") ? xml.getIntAttribute(attributeName + "_y") : xml.getIntAttribute(attributeName + "::y", defaultValue.getY());
    return {x, y};
}

template <>
void XmlParser::toXml<juce::StringPairArray>(juce::XmlElement& xml, juce::Identifier const& attributeName, juce::StringPairArray const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    if(child == nullptr)
    {
        return;
    }
    auto const& keys = value.getAllKeys();
    auto const& values = value.getAllValues();
    for(auto i = 0; i < std::min(keys.size(), values.size()); ++i)
    {
        auto subChild = child->createNewChildElement("pair");
        if(subChild != nullptr)
        {
            subChild->setAttribute("key", keys[i]);
            subChild->setAttribute("value", values[i]);
        }
    }
    xml.addChildElement(child.release());
}

template <>
auto XmlParser::fromXml<juce::StringPairArray>(juce::XmlElement const& xml, juce::Identifier const& attributeName, juce::StringPairArray const& defaultValue)
    -> juce::StringPairArray
{
    auto const* child = xml.getChildByName(attributeName);
    if(child == nullptr)
    {
        return defaultValue;
    }
    juce::StringPairArray value;
    for(auto const* subChild = child->getChildByName("pair"); subChild != nullptr; subChild = subChild->getNextElementWithTagName("pair"))
    {
        MiscWeakAssert(subChild->hasAttribute("key"));
        MiscWeakAssert(subChild->hasAttribute("value"));
        value.set(subChild->getStringAttribute("key", ""), subChild->getStringAttribute("value", ""));
    }
    return value;
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

        beginTest("vector");
        {
            juce::XmlElement xml("Test");
            std::vector<int> const value{0, 1, 2};
            XmlParser::toXml(xml, "vector", value);
            expect(value == XmlParser::fromXml(xml, "vector", std::vector<int>{}));
        }

        beginTest("std::map");
        {
            juce::XmlElement xml("Test");
            std::map<std::string, int> const value{{"v0", 0}, {"v1", 1}, {"v2", 2}};
            XmlParser::toXml(xml, "map", value);
            expect(value == XmlParser::fromXml(xml, "map", std::map<std::string, int>{}));
        }

        beginTest("juce::StringPairArray");
        {
            juce::XmlElement xml("Test");
            juce::StringPairArray value;
            value.set("1", "1");
            value.set("2", "2");
            value.set("john", "john");
            value.set("jimi", "jimi");
            XmlParser::toXml(xml, "StringPairArray", value);
            expect(value == XmlParser::fromXml(xml, "StringPairArray", juce::StringPairArray{}));
        }
    }
};

static XmlParserUnitTest xmlParserUnitTest;

MISC_FILE_END
