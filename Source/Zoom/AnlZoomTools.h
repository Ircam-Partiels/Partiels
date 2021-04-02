#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace Tools
    {
        Range getScaledVisibleRange(Accessor const& zoomAcsr, Range const& newGlobalRange);
    }
}

ANALYSE_FILE_END
