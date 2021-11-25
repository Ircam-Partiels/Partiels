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

ANALYSE_FILE_END
