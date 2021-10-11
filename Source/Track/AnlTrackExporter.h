#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Exporter
    {
        juce::Result fromPreset(Accessor& accessor, juce::File const& file);

        juce::Result toPreset(Accessor const& accessor, juce::File const& file);
        juce::Result toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::File const& file, int width, int height, std::atomic<bool> const& shouldAbort);
        juce::Result toCsv(Accessor const& accessor, juce::File const& file, bool includeHeader, char separator, std::atomic<bool> const& shouldAbort);
        juce::Result toJson(Accessor const& accessor, juce::File const& file, bool includeDescription, std::atomic<bool> const& shouldAbort);
        juce::Result toCue(Accessor const& accessor, juce::File const& file, std::atomic<bool> const& shouldAbort);
        juce::Result toBinary(Accessor const& accessor, juce::File const& file, std::atomic<bool> const& shouldAbort);
    } // namespace Exporter
} // namespace Track

ANALYSE_FILE_END
