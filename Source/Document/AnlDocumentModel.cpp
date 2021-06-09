#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

template <>
void XmlParser::toXml<Document::ReaderChannel>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::ReaderChannel const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "file", value.file);
        toXml(*child, "channel", value.channel);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Document::ReaderChannel>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::ReaderChannel const& defaultValue)
    -> Document::ReaderChannel
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Document::ReaderChannel value;
    value.file = fromXml(*child, "file", defaultValue.file);
    value.channel = fromXml(*child, "channel", defaultValue.channel);
    return value;
}

ANALYSE_FILE_END
