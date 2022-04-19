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
            , points
            , columns
        };
        // clang-format on

        bool hasPluginKey(Accessor const& acsr);
        bool hasResultFile(Accessor const& acsr);
        bool supportsWindowType(Accessor const& acsr);
        bool supportsBlockSize(Accessor const& acsr);
        bool supportsStepSize(Accessor const& acsr);
        DisplayType getDisplayType(Accessor const& acsr);

        float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);

        juce::String getInfoTooltip(Accessor const& acsr);
        juce::String getValueTootip(Accessor const& acsr, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time);
        juce::String getStateTootip(Accessor const& acsr, bool includeAnalysisState = true, bool includeRenderingState = true);

        std::optional<Zoom::Range> getValueRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getBinRange(Plugin::Description const& description);

        std::map<size_t, juce::Range<int>> getChannelVerticalRanges(Accessor const& acsr, juce::Rectangle<int> bounds);
        void paintChannels(Accessor const& acsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& separatorColour, std::function<void(juce::Rectangle<int>, size_t)> fn);
        void paintClippedImage(juce::Graphics& g, juce::Image const& image, juce::Rectangle<float> const& bounds);

        Results getResults(Plugin::Output const& output, std::vector<std::vector<Plugin::Result>> const& pluginResults, std::atomic<bool> const& shouldAbort);

        std::unique_ptr<juce::Component> createValueRangeEditor(Accessor& acsr);
        std::unique_ptr<juce::Component> createBinRangeEditor(Accessor& acsr);
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
