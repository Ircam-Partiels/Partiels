#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

void Track::Accessor::releaseResultsReadingAccess()
{
    anlStrongAssert(mNumReadingAccess > 0);
    --mNumReadingAccess;
}

bool Track::Accessor::acquireResultsReadingAccess()
{
    if(mRequireWrittingAccess.load())
    {
        return false;
    }
    ++mNumReadingAccess;
    return true;
}

bool Track::Accessor::canContinueToReadResults() const
{
    return mRequireWrittingAccess.load() == false;
}

void Track::Accessor::releaseResultsWrittingAccess()
{
    anlStrongAssert(mRequireWrittingAccess.load() == true);
    mRequireWrittingAccess = false;
}

void Track::Accessor::acquireResultsWrittingAccess()
{
    anlStrongAssert(mRequireWrittingAccess.load() == false);
    mRequireWrittingAccess = true;
    while(mNumReadingAccess > 0)
    {
    }
}

template<>
void XmlParser::toXml<Track::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::ColourSet const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "map", value.map);
        toXml(*child, "foreground", value.foreground);
        toXml(*child, "background", value.background);
        xml.addChildElement(child.release());
    }
}

template<>
auto XmlParser::fromXml<Track::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::ColourSet const& defaultValue)
-> Track::ColourSet
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Track::ColourSet value;
    value.map = fromXml(*child, "map", defaultValue.map);
    value.foreground = fromXml(*child, "foreground", defaultValue.foreground);
    value.background = fromXml(*child, "background", defaultValue.background);
    return value;
}

ANALYSE_FILE_END
