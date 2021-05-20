#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Tools::DisplayType Track::Tools::getDisplayType(Accessor const& acsr)
{
    auto const results = acsr.getAttr<AttrType::results>();
    if(results.isEmpty())
    {
        if(results.getMarkers() != nullptr)
        {
            return DisplayType::markers;
        }
        if(results.getPoints() != nullptr)
        {
            return DisplayType::points;
        }
        if(results.getColumns() != nullptr)
        {
            return DisplayType::columns;
        }
    }
    auto const& output = acsr.getAttr<AttrType::description>().output;
    if(output.hasFixedBinCount)
    {
        switch(output.binCount)
        {
            case 0:
            {
                return DisplayType::markers;
            }
            break;
            case 1:
            {
                return DisplayType::points;
            }
            break;
            default:
            {
                return DisplayType::columns;
            }
            break;
        }
    }
    return DisplayType::points;
}

float Track::Tools::valueToPixel(float value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds)
{
    return (1.0f - static_cast<float>((static_cast<double>(value) - valueRange.getStart()) / valueRange.getLength())) * bounds.getHeight() + bounds.getY();
}

float Track::Tools::secondsToPixel(double seconds, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<float>((seconds - timeRange.getStart()) / timeRange.getLength()) * bounds.getWidth() + bounds.getX();
}

double Track::Tools::pixelToSeconds(float position, juce::Range<double> const& timeRange, juce::Rectangle<float> const& bounds)
{
    return static_cast<double>(position - bounds.getX()) / static_cast<double>(bounds.getWidth()) * timeRange.getLength() + timeRange.getStart();
}

std::optional<std::string> Track::Tools::getValue(Results::SharedMarkers results, size_t channel, Zoom::Range const& globalRange, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, globalRange, time);
    if(it != channelResults.cend())
    {
        return std::get<2>(*it);
    }
    return {};
}

std::optional<float> Track::Tools::getValue(Results::SharedPoints results, size_t channel, Zoom::Range const& globalRange, double time)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const first = findFirstAt(channelResults, globalRange, time);
    if(first == channelResults.cend())
    {
        return {};
    }
    auto const second = std::next(first);
    auto const end = std::get<0>(*first) + std::get<1>(*first);
    if(second == channelResults.cend() || time < end || !std::get<2>(*second).has_value())
    {
        return std::get<2>(*first);
    }
    auto const next = std::get<0>(*second);
    if(next <= end || !std::get<2>(*first).has_value())
    {
        return std::get<2>(*second);
    }
    auto const ratio = (time - end) / (next - end);
    return (1.0 - ratio) * *std::get<2>(*first) + ratio * *std::get<2>(*second);
}

std::optional<float> Track::Tools::getValue(Results::SharedColumns results, size_t channel, Zoom::Range const& globalRange, double time, size_t bin)
{
    if(results == nullptr || results->size() <= channel || results->at(channel).empty())
    {
        return {};
    }
    auto const& channelResults = results->at(channel);
    auto const it = findFirstAt(channelResults, globalRange, time);
    if(it == channelResults.cend() || std::get<2>(*it).empty())
    {
        return {};
    }
    auto const& column = std::get<2>(*it);
    anlWeakAssert(bin < column.size());
    if(bin >= column.size())
    {
        return {};
    }
    return column[bin];
}

juce::String Track::Tools::getValueTootip(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Component const& component, int y, double time)
{
    auto const results = accessor.getAttr<AttrType::results>();
    auto const& timeGlobalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    if(results.isEmpty() || timeGlobalRange.isEmpty())
    {
        return "-";
    }

    auto getChannel = [&]() -> std::optional<std::tuple<size_t, juce::Range<int>>>
    {
        auto bounds = component.getLocalBounds();
        if(!bounds.getVerticalRange().contains(y - 1))
        {
            return {};
        }

        auto const channelLayout = accessor.getAttr<AttrType::channelsLayout>();
        auto const numChannels = static_cast<size_t>(std::count(channelLayout.cbegin(), channelLayout.cend(), true));
        if(numChannels == 0_z)
        {
            return {};
        }

        auto const fullHeight = component.getHeight();
        auto const channelHeight = (fullHeight - static_cast<int>(numChannels) + 1) / static_cast<int>(numChannels);

        auto channelCounter = 0_z;
        for(auto channel = 0_z; channel < channelLayout.size(); ++channel)
        {
            if(channelLayout[channel])
            {
                ++channelCounter;
                auto range = bounds.removeFromTop(channelHeight + 1).getVerticalRange();
                if(range.contains(y - 1))
                {
                    range = range.withEnd(range.getEnd() - (channelCounter == numChannels ? 1 : 0));
                    if(range.contains(y - 1))
                    {
                        return std::make_tuple(channel, range);
                    }
                    return {};
                }
            }
        }
        return {};
    };

    auto const channel = getChannel();
    if(!channel.has_value())
    {
        return "-";
    }

    if(auto markers = results.getMarkers())
    {
        auto const value = getValue(markers, std::get<0>(*channel), timeGlobalRange, time);
        if(value.has_value())
        {
            return *value + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto points = results.getPoints())
    {
        auto const value = getValue(points, std::get<0>(*channel), timeGlobalRange, time);
        if(value.has_value())
        {
            return juce::String(*value, 2) + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    if(auto columns = results.getColumns())
    {
        auto getBinIndex = [&]() -> std::optional<size_t>
        {
            auto const binVisibleRange = accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
            auto const start = std::get<1>(*channel).getStart();
            auto const length = std::get<1>(*channel).getLength();
            if(length <= 0 || binVisibleRange.isEmpty())
            {
                return {};
            }
            auto const revertY = std::max(length - (y - start) - 1, 0);
            anlWeakAssert(revertY <= length - 1);
            auto const ratio = static_cast<double>(std::min(revertY, length - 1)) / static_cast<double>(length - 1);
            return static_cast<size_t>(std::floor(ratio * binVisibleRange.getLength() + binVisibleRange.getStart()));
        };

        auto binIndex = getBinIndex();
        if(!binIndex.has_value())
        {
            return "-";
        }

        auto const value = getValue(columns, std::get<0>(*channel), timeGlobalRange, time, *binIndex);
        if(value.has_value())
        {
            auto const& output = accessor.getAttr<AttrType::description>().output;
            auto getBinName = [&]()
            {
                if(*binIndex >= output.binNames.size())
                {
                    return "[" + juce::String(*binIndex) + "]";
                }
                return "[" + juce::String(*binIndex) + " - " + output.binNames[*binIndex] + "]";
            };
            return getBinName() + juce::String(*value, 2) + accessor.getAttr<AttrType::description>().output.unit;
        }
        return "-";
    }
    return "-";
}

juce::String Track::Tools::getStateTootip(Accessor const& acsr)
{
    auto const& state = acsr.getAttr<AttrType::processing>();
    auto const& warnings = acsr.getAttr<AttrType::warnings>();

    auto const name = acsr.getAttr<AttrType::name>() + ": ";
    if(std::get<0>(state))
    {
        return name + juce::translate("analysing... (" + juce::String(static_cast<int>(std::round(std::get<1>(state) * 100.f))) + "%)");
    }
    else if(std::get<2>(state))
    {
        return name + juce::translate("rendering... (" + juce::String(static_cast<int>(std::round(std::get<3>(state) * 100.f))) + "%)");
    }
    switch(warnings)
    {
        case WarningType::none:
            return name + juce::translate("analysis and rendering successfully completed!");
        case WarningType::plugin:
            return name + juce::translate("analysis failed: the plugin cannot be found or allocated!");
        case WarningType::state:
            return name + juce::translate("analysis failed: the step size or the block size might not be supported!");
    }
    return name + juce::translate("analysis and rendering successfully completed!");
}

std::optional<Zoom::Range> Track::Tools::getValueRange(Plugin::Description const& description)
{
    auto const& output = description.output;
    if(!output.hasKnownExtents)
    {
        return std::optional<Zoom::Range>();
    }
    anlWeakAssert(std::isfinite(output.minValue) && std::isfinite(output.maxValue));
    if(!std::isfinite(output.minValue) || !std::isfinite(output.maxValue))
    {
        return std::optional<Zoom::Range>();
    }
    return Zoom::Range(static_cast<double>(output.minValue), static_cast<double>(output.maxValue));
}

std::optional<Zoom::Range> Track::Tools::getValueRange(Results const& results)
{
    if(results.isEmpty())
    {
        return std::optional<Zoom::Range>();
    }
    if(results.getMarkers() != nullptr)
    {
        return Zoom::Range(0.0, 0.0);
    }
    auto const points = results.getPoints();
    if(points != nullptr)
    {
        if(points->empty())
        {
            return std::optional<Zoom::Range>();
        }
        auto const& channel = points->at(0);
        auto const [min, max] = std::minmax_element(channel.cbegin(), channel.cend(), [](auto const& lhs, auto const& rhs)
                                                    {
                                                        if(!std::get<2>(lhs).has_value())
                                                        {
                                                            return true;
                                                        }
                                                        if(!std::get<2>(rhs).has_value())
                                                        {
                                                            return false;
                                                        }
                                                        return *std::get<2>(lhs) < *std::get<2>(rhs);
                                                    });
        if(min == channel.cend() || max == channel.cend())
        {
            return std::optional<Zoom::Range>();
        }
        return Zoom::Range(*std::get<2>(*min), *std::get<2>(*max));
    }
    auto const columns = results.getColumns();
    if(columns != nullptr)
    {
        if(columns->empty())
        {
            return std::optional<Zoom::Range>();
        }
        auto const& channel = columns->at(0);
        if(channel.empty())
        {
            return std::optional<Zoom::Range>();
        }
        auto const [min, max] = std::minmax_element(std::get<2>(channel.front()).cbegin(), get<2>(channel.front()).cend());
        if(min == std::get<2>(channel.front()).cend() || max == std::get<2>(channel.front()).cend())
        {
            return std::optional<Zoom::Range>();
        }
        anlWeakAssert(std::isfinite(*min) && std::isfinite(*max));
        if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
        {
            return std::optional<Zoom::Range>();
        }

        return std::accumulate(std::next(channel.cbegin()), channel.cend(), Zoom::Range{*min, *max}, [](auto const& range, auto const& column)
                               {
                                   auto const& values = std::get<2>(column);
                                   if(values.empty())
                                   {
                                       return range;
                                   }
                                   auto const [min, max] = std::minmax_element(values.cbegin(), values.cend());
                                   if(min == values.cend() || max == values.cend())
                                   {
                                       return range;
                                   }
                                   anlWeakAssert(std::isfinite(*min) && std::isfinite(*max) && !std::isnan(*min) && !std::isnan(*max));
                                   if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
                                   {
                                       return range;
                                   }
                                   return range.getUnionWith({*min, *max});
                               });
    }
    return std::optional<Zoom::Range>();
}

std::optional<Zoom::Range> Track::Tools::getBinRange(Plugin::Description const& description)
{
    auto const& output = description.output;
    if(!output.hasFixedBinCount)
    {
        return std::optional<Zoom::Range>();
    }
    return Zoom::Range(0.0, static_cast<double>(output.binCount));
}

std::optional<Zoom::Range> Track::Tools::getBinRange(Results const& results)
{
    if(results.isEmpty())
    {
        return std::optional<Zoom::Range>();
    }
    if(results.getMarkers() != nullptr)
    {
        return Zoom::Range(0.0, 0.0);
    }
    if(results.getPoints() != nullptr)
    {
        return Zoom::Range(0.0, 1.0);
    }
    auto const columns = results.getColumns();
    if(columns == nullptr || columns->empty())
    {
        return std::optional<Zoom::Range>();
    }
    auto const& channel = columns->at(0);
    return Zoom::Range(0.0, static_cast<double>(channel.size()));
}

ANALYSE_FILE_END
