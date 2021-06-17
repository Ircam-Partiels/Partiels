#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    namespace Exporter
    {
        juce::Result toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height, std::atomic<bool> const& shouldAbort);
    } // namespace Exporter
} // namespace Group

ANALYSE_FILE_END
