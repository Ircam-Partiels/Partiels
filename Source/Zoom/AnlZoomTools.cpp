#include "AnlZoomTools.h"

ANALYSE_FILE_BEGIN

Zoom::Range Zoom::Tools::getScaledVisibleRange(Zoom::Accessor const& acsr, Zoom::Range const& newGlobalRange)
{
    auto const globalRange = acsr.getAttr<AttrType::globalRange>();
    if(globalRange.isEmpty())
    {
        return newGlobalRange;
    }
    auto const visiblelRange = acsr.getAttr<AttrType::visibleRange>();
    auto const startRatio = (visiblelRange.getStart() - globalRange.getStart()) / globalRange.getLength();
    auto const endRatio = (visiblelRange.getEnd() - globalRange.getStart()) / globalRange.getLength();
    auto const newStart = startRatio * newGlobalRange.getLength() + newGlobalRange.getStart();
    auto const newEnd = endRatio * newGlobalRange.getLength() + newGlobalRange.getStart();
    return {newStart, newEnd};
}

ANALYSE_FILE_END
