#include "AnlTransportTools.h"

ANALYSE_FILE_BEGIN

bool Transport::Tools::canRewindPlayhead(Accessor const& accessor)
{
    auto const loopRange = accessor.getAttr<AttrType::loopRange>();
    auto const isLooping = accessor.getAttr<AttrType::looping>();
    auto const stopAtEnd = accessor.getAttr<AttrType::stopAtLoopEnd>();
    auto const shouldUseLoopRange = !loopRange.isEmpty() && (isLooping || stopAtEnd);
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    auto const start = shouldUseLoopRange && position > loopRange.getStart() ? loopRange.getStart() : 0.0;
    return std::abs(position - start) > std::numeric_limits<double>::epsilon();
}

void Transport::Tools::rewindPlayhead(Accessor& accessor)
{
    auto const isPlaying = accessor.getAttr<AttrType::playback>();
    auto const loopRange = accessor.getAttr<AttrType::loopRange>();
    auto const isLooping = accessor.getAttr<AttrType::looping>();
    auto const stopAtEnd = accessor.getAttr<AttrType::stopAtLoopEnd>();
    auto const shouldUseLoopRange = !loopRange.isEmpty() && (isLooping || stopAtEnd);
    auto const position = accessor.getAttr<AttrType::startPlayhead>();
    auto const start = shouldUseLoopRange && position > loopRange.getStart() ? loopRange.getStart() : 0.0;
    if(isPlaying)
    {
        accessor.setAttr<AttrType::playback>(false, NotificationType::synchronous);
    }
    accessor.setAttr<AttrType::startPlayhead>(start, NotificationType::synchronous);
    if(isPlaying)
    {
        accessor.setAttr<AttrType::playback>(true, NotificationType::synchronous);
    }
}

double Transport::Tools::getPreviousTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    if(markers.empty() || !accessor.getAttr<AttrType::magnetize>())
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

double Transport::Tools::getNextTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    if(markers.empty() || !accessor.getAttr<AttrType::magnetize>())
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

double Transport::Tools::getNearestTime(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    if(markers.empty() || !accessor.getAttr<AttrType::magnetize>())
    {
        return range.has_value() ? range->clipValue(time) : time;
    }
    if(time <= *markers.cbegin())
    {
        if(range.has_value())
        {
            auto const next = range->clipValue(*markers.cbegin());
            auto const previous = range->getStart();
            return std::abs(next - time) <= std::abs(previous - time) ? next : previous;
        }
        return *markers.cbegin();
    }
    if(time > *markers.crbegin())
    {
        if(range.has_value())
        {
            auto const previous = range->clipValue(*markers.crbegin());
            auto const next = range->getEnd();
            return std::abs(next - time) <= std::abs(previous - time) ? next : previous;
        }
        return *markers.crbegin();
    }
    auto const nextIt = std::lower_bound(markers.cbegin(), markers.cend(), time);
    auto const previousIt = std::prev(nextIt);
    if(range.has_value())
    {
        auto const next = range->clipValue(*nextIt);
        auto const previous = range->clipValue(*previousIt);
        return std::abs(next - time) <= std::abs(previous - time) ? next : previous;
    }
    return std::abs(*nextIt - time) <= std::abs(*previousIt - time) ? *nextIt : *previousIt;
}

juce::Range<double> Transport::Tools::getTimeRange(Accessor const& accessor, double time, std::optional<juce::Range<double>> const& range)
{
    auto const& markers = accessor.getAttr<AttrType::markers>();
    if(markers.empty() || !accessor.getAttr<AttrType::magnetize>())
    {
        return range.has_value() ? *range : juce::Range<double>{time, time};
    }
    if(time <= *markers.cbegin())
    {
        if(range.has_value())
        {
            auto const next = range->clipValue(*markers.cbegin());
            auto const previous = range->getStart();
            return {previous, next};
        }
        return {*markers.cbegin(), *markers.cbegin()};
    }
    if(time > *markers.crbegin())
    {
        if(range.has_value())
        {
            auto const previous = range->clipValue(*markers.crbegin());
            auto const next = range->getEnd();
            return {previous, next};
        }
        return {*markers.crbegin(), *markers.crbegin()};
    }
    auto const nextIt = std::lower_bound(markers.cbegin(), markers.cend(), time);
    auto const previousIt = std::prev(nextIt);
    if(range.has_value())
    {
        auto const next = range->clipValue(*nextIt);
        auto const previous = range->clipValue(*previousIt);
        return {previous, next};
    }
    return {*previousIt, *nextIt};
}

ANALYSE_FILE_END
