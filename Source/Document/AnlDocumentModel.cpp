#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

template<>
void XmlParser::toXml<Document::Group>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Group const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "identifier", value.identifier);
        toXml(*child, "name", value.name);
        toXml(*child, "height", value.height);
        toXml(*child, "colour", value.colour);
        toXml(*child, "open", value.open);
        toXml(*child, "layout", value.layout);
        xml.addChildElement(child.release());
    }
}

template<>
auto XmlParser::fromXml<Document::Group>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Group const& defaultValue)
-> Document::Group
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Document::Group value;
    value.identifier = fromXml(*child, "identifier", defaultValue.identifier);
    value.name = fromXml(*child, "name", defaultValue.name);
    value.height = fromXml(*child, "height", defaultValue.height);
    value.colour = fromXml(*child, "colour", defaultValue.colour);
    value.open = fromXml(*child, "open", defaultValue.open);
    value.layout = fromXml(*child, "layout", defaultValue.layout);
    return value;
}

ANALYSE_FILE_END
