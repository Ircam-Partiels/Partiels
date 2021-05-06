#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Tools
    {
        // clang-format off
        enum class DisplayType
        {
              markers
            , segments
            , grid
        };
        // clang-format on

        DisplayType getDisplayType(Accessor const& acsr);

        float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double realTimeToSeconds(Vamp::RealTime const& rt);
        Vamp::RealTime secondsToRealTime(double seconds);
        Vamp::RealTime getEndRealTime(Plugin::Result const& rt);

        //! @brief Optimized method to get the iterator at a given time
        std::vector<Plugin::Result>::const_iterator getIteratorAt(std::vector<Plugin::Result> const& results, Zoom::Range const& globalRange, double time);

        juce::String getMarkerText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, double timeEpsilon);
        juce::String getSegmentText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time);
        juce::String getGridText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, size_t bin);
        juce::String getResultText(Accessor const& acsr, Zoom::Range const& globalRange, double time, size_t bin, double timeEpsilon);
        juce::String getProcessingTooltip(Accessor const& acsr);

        std::optional<Zoom::Range> getValueRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getValueRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results);
        std::optional<Zoom::Range> getBinRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getBinRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results);
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
