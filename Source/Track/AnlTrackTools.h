#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Tools
    {
        bool hasPluginKey(Accessor const& acsr);
        bool hasResultFile(Accessor const& acsr);
        bool supportsWindowType(Accessor const& acsr);
        bool supportsBlockSize(Accessor const& acsr);
        bool supportsStepSize(Accessor const& acsr);
        bool supportsInputTrack(Accessor const& acsr);
        std::optional<FrameType> getFrameType(Plugin::Output const& output);
        std::optional<FrameType> getFrameType(Accessor const& acsr);

        bool canZoomIn(Accessor const& accessor);
        bool canZoomOut(Accessor const& accessor);
        void zoomIn(Accessor& accessor, double ratio, NotificationType notification);
        void zoomOut(Accessor& accessor, double ratio, NotificationType notification);

        float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float pixelToValue(float position, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);

        juce::String getUnit(Accessor const& acsr);
        juce::String getBinName(Accessor const& acsr, size_t index, bool prependIndex);

        std::optional<Zoom::Range> getResultRange(Accessor const& accessor);
        std::optional<Zoom::Range> getValueRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getBinRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getExtraRange(Accessor const& accessor, size_t index);

        bool isSelected(Accessor const& acsr);
        std::set<size_t> getSelectedChannels(Accessor const& acsr);
        std::optional<size_t> getChannel(Accessor const& acsr, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator);
        std::optional<std::tuple<size_t, juce::Range<int>>> getChannelVerticalRange(Accessor const& acsr, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator);
        std::map<size_t, juce::Range<int>> getChannelVerticalRanges(Accessor const& acsr, juce::Rectangle<int> bounds);

        Results convert(Plugin::Output const& output, std::vector<std::vector<Plugin::Result>>& pluginResults, std::atomic<bool> const& shouldAbort);
        std::vector<std::vector<Plugin::Result>> convert(Plugin::Input const& input, Results const& results);

        std::unique_ptr<juce::Component> createValueRangeEditor(Accessor& acsr);
        std::unique_ptr<juce::Component> createBinRangeEditor(Accessor& acsr);
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
