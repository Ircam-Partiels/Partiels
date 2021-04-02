#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Tools
    {
        float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double realTimeToSeconds(Vamp::RealTime const& rt);
        Vamp::RealTime secondsToRealTime(double seconds);
        Vamp::RealTime getEndRealTime(Plugin::Result const& rt);

        juce::String getMarkerText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time);
        juce::String getSegmentText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time);
        juce::String getGridText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time, size_t bin);
    };
}

ANALYSE_FILE_END
