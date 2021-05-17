#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Exporter
    {
    public:
        static juce::Result fromPreset(Accessor& accessor, juce::File const& file);

        static juce::Result toPreset(Accessor const& accessor, juce::File const& file);
        static juce::Result toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height);
        static juce::Result toCsv(Accessor const& accessor, juce::File const& file, bool includeHeader, char separator);
        static juce::Result toJson(Accessor const& accessor, juce::File const& file);
    };
} // namespace Track

ANALYSE_FILE_END
