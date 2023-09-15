#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Tools
    {
        juce::String getInfoTooltip(Accessor const& acsr);
        juce::String getZoomTootip(Accessor const& acsr, juce::Component const& component, int y);
        juce::StringArray getValueTootip(Accessor const& acsr, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time);
        juce::String getStateTootip(Accessor const& acsr, bool includeAnalysisState = true, bool includeRenderingState = true);
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
