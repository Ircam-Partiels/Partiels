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

        template <typename T>
        typename T::const_iterator findFirstAt(T const& results, Zoom::Range const& globalRange, double time)
        {
            anlWeakAssert(!globalRange.isEmpty());
            if(globalRange.isEmpty())
            {
                return results.cend();
            }

            auto const timeRatioPosition = std::max(std::min((time - globalRange.getStart()) / globalRange.getLength(), 1.0), 0.0);
            auto const expectedIndex = static_cast<long>(std::ceil(timeRatioPosition * static_cast<double>(results.size() - 1_z)));

            auto const expectedIt = std::next(results.cbegin(), expectedIndex);
            anlWeakAssert(expectedIt != results.cend());
            if(expectedIt == results.cend())
            {
                return results.cend();
            }
            auto const position = std::get<0>(*expectedIt);
            if(position >= time && position + std::get<1>(*expectedIt) <= time)
            {
                return expectedIt;
            }
            else if(position >= time)
            {
                return std::find_if(std::make_reverse_iterator(expectedIt), results.crend(), [&](auto const& result)
                                    {
                                        return std::get<0>(result) + std::get<1>(result) <= time;
                                    })
                    .base();
            }
            else
            {
                auto const it = std::find_if(expectedIt, results.cend(), [&](auto const& result)
                                             {
                                                 return std::get<0>(result) + std::get<1>(result) >= time;
                                             });
                if(it == results.cend())
                {
                    return std::prev(it);
                }
                if(std::get<0>(*it) > time && it != results.cbegin())
                {
                    return std::prev(it);
                }
                return it;
            }
        }

        std::optional<std::string> getValue(Results::SharedMarkers results, Zoom::Range const& globalRange, double time, double timeEpsilon);
        std::optional<float> getValue(Results::SharedPoints results, Zoom::Range const& globalRange, double time);
        std::optional<float> getValue(Results::SharedColumns results, Zoom::Range const& globalRange, double time, size_t bin);

        juce::String getText(Results::SharedMarkers results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, double timeEpsilon);
        juce::String getText(Results::SharedPoints results, Plugin::Output const& output, Zoom::Range const& globalRange, double time);
        juce::String getText(Results::SharedColumns results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, size_t bin);

        juce::String getResultText(Accessor const& acsr, Zoom::Range const& globalRange, double time, size_t bin, double timeEpsilon);
        juce::String getProcessingTooltip(Accessor const& acsr);

        std::optional<Zoom::Range> getValueRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getValueRange(Results const& results);
        std::optional<Zoom::Range> getBinRange(Plugin::Description const& description);
        std::optional<Zoom::Range> getBinRange(Results const& results);
    } // namespace Tools
} // namespace Track

ANALYSE_FILE_END
