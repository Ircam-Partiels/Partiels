#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Exporter
    {
    public:
        static juce::Result toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height);
    };
} // namespace Group

ANALYSE_FILE_END
