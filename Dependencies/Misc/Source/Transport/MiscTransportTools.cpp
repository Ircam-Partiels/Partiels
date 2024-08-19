#include "MiscTransportTools.h"

MISC_FILE_BEGIN

bool Transport::Tools::canRewindPlayhead(Accessor const& accessor)
{
    auto const loopRange = accessor.getAttr<AttrType::loopRange>();
    auto const isLooping = accessor.getAttr<AttrType::looping>();
    auto const stopAtEnd = accessor.getAttr<AttrType::stopAtLoopEnd>();
    auto const shouldUseLoopRange = !loopRange.isEmpty() && (isLooping || stopAtEnd);
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    auto const start = shouldUseLoopRange ? loopRange.getStart() : 0.0;
    return std::abs(position - start) > std::numeric_limits<double>::epsilon();
}

bool Transport::Tools::canMovePlayheadBackward(Accessor const& accessor, std::optional<juce::Range<double>> const& range)
{
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    return std::abs(getPreviousTime(accessor, position, range) - position) > std::numeric_limits<double>::epsilon();
}

bool Transport::Tools::canMovePlayheadForward(Accessor const& accessor, std::optional<juce::Range<double>> const& range)
{
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    return std::abs(getNextTime(accessor, position, range) - position) > std::numeric_limits<double>::epsilon();
}

void Transport::Tools::rewindPlayhead(Accessor& accessor, NotificationType notification)
{
    auto const isPlaying = accessor.getAttr<AttrType::playback>();
    auto const loopRange = accessor.getAttr<AttrType::loopRange>();
    auto const isLooping = accessor.getAttr<AttrType::looping>();
    auto const stopAtEnd = accessor.getAttr<AttrType::stopAtLoopEnd>();
    auto const shouldUseLoopRange = !loopRange.isEmpty() && (isLooping || stopAtEnd);
    auto const start = shouldUseLoopRange ? loopRange.getStart() : 0.0;
    if(isPlaying)
    {
        accessor.setAttr<AttrType::playback>(false, notification);
    }
    accessor.setAttr<AttrType::startPlayhead>(start, notification);
    if(isPlaying)
    {
        accessor.setAttr<AttrType::playback>(true, notification);
    }
}

void Transport::Tools::movePlayheadBackward(Accessor& accessor, std::optional<juce::Range<double>> const& range, NotificationType notification)
{
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    accessor.setAttr<AttrType::startPlayhead>(getPreviousTime(accessor, position, range), notification);
}

void Transport::Tools::movePlayheadForward(Accessor& accessor, std::optional<juce::Range<double>> const& range, NotificationType notification)
{
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    accessor.setAttr<AttrType::startPlayhead>(getNextTime(accessor, position, range), notification);
}

double Transport::Tools::getPreviousTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range, bool forceMagnetize)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    auto const magnetize = forceMagnetize || accessor.getAttr<AttrType::magnetize>();
    if(markers.empty() || !magnetize)
    {
        return range.has_value() ? range->getStart() : time;
    }
    if(time <= *markers.cbegin())
    {
        return range.has_value() ? range->getStart() : *markers.cbegin();
    }
    if(time > *markers.crbegin())
    {
        return range.has_value() ? range->clipValue(*markers.crbegin()) : *markers.crbegin();
    }
    auto const value = *std::prev(std::lower_bound(markers.cbegin(), markers.cend(), time));
    return range.has_value() ? range->clipValue(value) : value;
}

double Transport::Tools::getNextTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range, bool forceMagnetize)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    auto const magnetize = forceMagnetize || accessor.getAttr<AttrType::magnetize>();
    if(markers.empty() || !magnetize)
    {
        return range.has_value() ? range->getEnd() : time;
    }
    if(time < *markers.cbegin())
    {
        return range.has_value() ? range->clipValue(*markers.cbegin()) : *markers.cbegin();
    }
    if(time >= *markers.crbegin())
    {
        return range.has_value() ? range->getEnd() : *markers.crbegin();
    }
    auto const value = *std::upper_bound(markers.cbegin(), markers.cend(), time);
    return range.has_value() ? range->clipValue(value) : value;
}

MISC_FILE_END
