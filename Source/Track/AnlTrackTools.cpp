#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Tools::DisplayType Track::Tools::getDisplayType(Accessor const& acsr)
{
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
                return DisplayType::segments;
            }
            break;
            default:
            {
                return DisplayType::grid;
            }
            break;
        }
    }
    return DisplayType::segments;
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

double Track::Tools::realTimeToSeconds(Vamp::RealTime const& rt)
{
    return static_cast<double>(rt.sec) + static_cast<double>(rt.nsec) / 1000000000.0;
}

Vamp::RealTime Track::Tools::secondsToRealTime(double seconds)
{
    auto const secondint = floor(seconds);
    return Vamp::RealTime(static_cast<int>(secondint), static_cast<int>(std::round((seconds - secondint) * 1000000000.0)));
}

Vamp::RealTime Track::Tools::getEndRealTime(Plugin::Result const& rt)
{
    anlWeakAssert(rt.hasTimestamp);
    return rt.hasDuration ? rt.timestamp + rt.duration : rt.timestamp;
}

std::vector<Plugin::Result>::const_iterator Track::Tools::getIteratorAt(std::vector<Plugin::Result> const& results, Zoom::Range const& globalRange, double time)
{
    anlWeakAssert(!globalRange.isEmpty());
    if(globalRange.isEmpty())
    {
        return results.cend();
    }

    auto const timeRatioPosition = std::max(std::min((time - globalRange.getStart()) / globalRange.getLength(), 1.0), 0.0);
    auto const expectedIndex = static_cast<long>(std::ceil(timeRatioPosition * static_cast<double>(results.size() - 1_z)));
    auto const rtStart = Tools::secondsToRealTime(time);

    auto const expectedIt = std::next(results.cbegin(), expectedIndex);
    anlStrongAssert(expectedIt != results.cend());
    auto const position = Tools::getEndRealTime(*expectedIt);
    
    if(position == rtStart)
    {
        return expectedIt;
    }
    else if(position >= rtStart)
    {
        return std::find_if(std::make_reverse_iterator(expectedIt), results.crend(), [&](Plugin::Result const& result)
                            {
                                return Tools::getEndRealTime(result) <= rtStart;
                            })
            .base();
    }
    else
    {
        auto const it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
                                     {
                                         return Tools::getEndRealTime(result) >= rtStart;
                                     });
        if(it->timestamp > rtStart && it != results.cbegin())
        {
            return std::prev(it);
        }
        return it;
    }
}

juce::String Track::Tools::getMarkerText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, double timeEpsilon)
{
    auto const it = getIteratorAt(results, globalRange, time - timeEpsilon);
    if(it != results.cend() && getEndRealTime(*it) <= secondsToRealTime(time))
    {
        return it->label.empty() ? output.unit : (it->label + output.unit);
    }
    return "-";
}

juce::String Track::Tools::getSegmentText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time)
{
    auto const first = getIteratorAt(results, globalRange, time);
    if(first == results.cend())
    {
        return "-";
    }
    auto const second = std::next(first);
    if(second == results.cend() || second->values.empty())
    {
        auto const value = (first->values.empty() ? "-" : juce::String(first->values.at(0), 2)) + output.unit;
        auto const label = first->label.empty() ? "" : ("(" + first->label + ")");
        return value + " " + label;
    }
    if(first->values.empty())
    {
        auto const value = juce::String(second->values.at(0), 2) + output.unit;
        auto const label = second->label.empty() ? "" : ("(" + second->label + ")");
        return value + " " + label;
    }
    auto const end = realTimeToSeconds(getEndRealTime(*first));
    if(time < end)
    {
        auto const value = first->values.empty() ? "-" : juce::String(first->values.at(0), 2) + output.unit;
        auto const label = first->label.empty() ? "" : ("(" + first->label + ")");
        return value + " " + label;
    }
    auto const next = realTimeToSeconds(second->timestamp);
    if(next <= end)
    {
        auto const value = juce::String(second->values.at(0), 2) + output.unit;
        auto const label = second->label.empty() ? "" : ("(" + second->label + ")");
        return value + " " + label;
    }
    auto const ratio = static_cast<float>((time - end) / (next - end));
    auto const value = juce::String((1.0f - ratio) * first->values.at(0) + ratio * second->values.at(0), 2) + output.unit;
    auto const label = second->label.empty() ? "" : ("(" + second->label + ")");
    return value + " " + label;
}

juce::String Track::Tools::getGridText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, Zoom::Range const& globalRange, double time, size_t bin)
{
    auto const it = getIteratorAt(results, globalRange, time);
    if(it != results.cend() || it->values.empty())
    {
        return "-";
    }
    anlWeakAssert(bin < it->values.size());
    if(bin >= it->values.size())
    {
        return "-";
    }
    auto const value = juce::String(it->values[bin], 2) + output.unit;
    auto const label = it->label.empty() ? "" : ("(" + it->label + ")");
    return value + " " + label;
}

juce::String Track::Tools::getResultText(Accessor const& acsr, Zoom::Range const& globalRange, double time, size_t bin, double timeEpsilon)
{
    auto const results = acsr.getAttr<AttrType::results>();
    if(results == nullptr || results->empty() || globalRange.isEmpty())
    {
        return "-";
    }
    auto const& output = acsr.getAttr<AttrType::description>().output;
    switch(getDisplayType(acsr))
    {
        case DisplayType::markers:
        {
            return Tools::getMarkerText(*results, output, globalRange, time, timeEpsilon);
        }
        break;
        case DisplayType::segments:
        {
            return Tools::getSegmentText(*results, output, globalRange, time);
        }
        break;
        case DisplayType::grid:
        {
            auto const hasBinName = bin < output.binNames.size() && !output.binNames[bin].empty();
            auto const binName = "bin" + juce::String(bin) + (hasBinName ? ("-" + output.binNames[bin]) : "");
            return "(" + binName + ") " + Tools::getGridText(*results, output, globalRange, time, bin);
        }
        break;
    }
    anlWeakAssert(false);
    return "";
}

juce::String Track::Tools::getProcessingTooltip(Accessor const& acsr)
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

std::optional<Zoom::Range> Track::Tools::getValueRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results)
{
    if(results == nullptr || results->empty())
    {
        return std::optional<Zoom::Range>();
    }
    auto it = std::find_if(results->cbegin(), results->cend(), [](auto const& v)
                           {
                               if(v.values.empty())
                               {
                                   return false;
                               }
                               auto const [min, max] = std::minmax_element(v.values.cbegin(), v.values.cend());
                               anlWeakAssert(std::isfinite(*min) && std::isfinite(*max) && !std::isnan(*min) && !std::isnan(*max));
                               if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
                               {
                                   return false;
                               }
                               return true;
                           });
    if(it == results->cend())
    {
        return std::optional<Zoom::Range>();
    }
    auto const [min, max] = std::minmax_element(it->values.cbegin(), it->values.cend());
    auto const firstRange = Zoom::Range{static_cast<double>(*min), static_cast<double>(*max)};
    return std::accumulate(std::next(it), results->cend(), firstRange, [](auto const r, auto const& v)
                           {
                               if(v.values.empty())
                               {
                                   return r;
                               }
                               auto const [min, max] = std::minmax_element(v.values.cbegin(), v.values.cend());
                               anlWeakAssert(std::isfinite(*min) && std::isfinite(*max) && !std::isnan(*min) && !std::isnan(*max));
                               if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
                               {
                                   return r;
                               }
                               return r.getUnionWith({static_cast<double>(*min), static_cast<double>(*max)});
                           });
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

std::optional<Zoom::Range> Track::Tools::getBinRange(std::shared_ptr<const std::vector<Plugin::Result>> const& results)
{
    if(results == nullptr || results->empty())
    {
        return std::optional<Zoom::Range>();
    }
    auto const firstRange = Zoom::Range::emptyRange(static_cast<double>(results->front().values.size()));
    return std::accumulate(results->cbegin() + 1, results->cend(), firstRange, [](auto const r, auto const& v)
                           {
                               return r.getUnionWith({static_cast<double>(v.values.size()), static_cast<double>(v.values.size())});
                           });
}

ANALYSE_FILE_END
