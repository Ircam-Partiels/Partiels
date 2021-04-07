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
        juce::String getResultText(Accessor const& acsr, double time, size_t bin = 0_z);
    
        std::optional<Zoom::Range> getValueRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getValueRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results);
        std::optional<Zoom::Range> getBinRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getBinRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results);
    }
}

ANALYSE_FILE_END
