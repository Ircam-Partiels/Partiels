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

        optional_ref<Zoom::Accessor const> getVerticalZoomAccessor(Accessor const& accessor, bool useLinkedZoom);
        optional_ref<Zoom::Accessor> getVerticalZoomAccessor(Accessor& accessor, bool useLinkedZoom);
        bool hasVerticalZoom(Accessor const& accessor);
        bool canZoomIn(Accessor const& accessor);
        bool canZoomOut(Accessor const& accessor);
        void zoomIn(Accessor& accessor, double ratio, NotificationType notification);
        void zoomOut(Accessor& accessor, double ratio, NotificationType notification);
        bool hasVerticalZoomInHertz(Accessor const& accessor);

        float valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float pixelToValue(float position, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds);
        float secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);
        double pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds);

        juce::String getUnit(Accessor const& acsr);
        juce::String getBinName(Accessor const& acsr, size_t index);

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

        static auto constexpr scaledFrequencyCenter = 440.0;
        static auto constexpr scaledMidiCenter = 69.0;
        static inline float getMidiFromHertz(float const& frequency)
        {
            return frequency > 0.0f ? std::log2(frequency / static_cast<float>(scaledFrequencyCenter)) * 12.0f + static_cast<float>(scaledMidiCenter) : 0.0f;
        }

        static inline float getHertzFromMidi(float const& midi)
        {
            return static_cast<float>(scaledFrequencyCenter) * std::pow(2.0f, (midi - static_cast<float>(scaledMidiCenter)) / 12.0f);
        }

        static inline double getMidiFromHertz(double const& frequency)
        {
            return frequency > 0.0 ? std::log2(frequency / scaledFrequencyCenter) * 12.0 + scaledMidiCenter : 0.0;
        }

        static inline double getHertzFromMidi(double const& midi)
        {
            return scaledFrequencyCenter * std::pow(2.0, (midi - scaledMidiCenter) / 12.0);
        }

        static inline juce::Range<double> getMidiFromHertz(juce::Range<double> const& range)
        {
            return {getMidiFromHertz(range.getStart()), getMidiFromHertz(range.getEnd())};
        }

        static inline juce::Range<double> getHertzFromMidi(juce::Range<double> const& range)
        {
            return {getHertzFromMidi(range.getStart()), getHertzFromMidi(range.getEnd())};
        }
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
