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

juce::String Track::Tools::getMarkerText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time, double timeEpsilon)
{
    auto const rt = secondsToRealTime(time - timeEpsilon);
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && getEndRealTime(result) >= rt;
    });
    if(it != results.cend() && getEndRealTime(*it) <= secondsToRealTime(time))
    {
        return it->label.empty() ? output.unit : (it->label + output.unit);
    }
    return "";
}

juce::String Track::Tools::getSegmentText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time)
{
    auto const rt = secondsToRealTime(time);
    auto const second = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && getEndRealTime(result) >= rt;
    });
    if(second == results.cend())
    {
        return "-";
    }
    auto const first = second != results.cbegin() ? std::prev(second) : results.cbegin();
    if(second->timestamp <= rt && getEndRealTime(*second) >= rt)
    {
        if(!second->values.empty())
        {
            auto const label = second->label.empty() ? output.unit : second->label;
            return juce::String(second->values[0], 2) + label;
        }
    }
    else if(first->timestamp <= rt && getEndRealTime(*first) >= rt)
    {
        if(!first->values.empty())
        {
            auto const label = first->label.empty() ? output.unit : first->label;
            return juce::String(first->values[0], 2) + label;
        }
    }
    else if(first != second && first->hasTimestamp)
    {
        if(!first->values.empty() && !second->values.empty())
        {
            auto const start = realTimeToSeconds(getEndRealTime(*first));
            auto const end = realTimeToSeconds(second->timestamp);
            anlStrongAssert(end > start);
            if(end <= start)
            {
                return "-";
            }
            auto const ratio = static_cast<float>((time - start) / (end - start));
            auto const value = (1.0f - ratio) * first->values[0] + ratio * second->values[0];
            auto const label = second->label.empty() ? output.unit : second->label;
            return juce::String(value, 2) + label;
        }
    }
    return "-";
}

juce::String Track::Tools::getGridText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time, size_t bin)
{
    auto const rt = secondsToRealTime(time);
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && getEndRealTime(result) >= rt;
    });
    if(it == results.cend() || it->values.empty())
    {
        return "-";
    }
    anlStrongAssert(bin < it->values.size());
    if(bin >= it->values.size())
    {
        return "-";
    }
    auto const label = it->label.empty() ? output.unit : it->label;
    return juce::String(it->values[bin], 2) + label;
}

juce::String Track::Tools::getResultText(Accessor const& acsr, double time, size_t bin, double timeEpsilon)
{
    auto const results = acsr.getAttr<AttrType::results>();
    auto const& output = acsr.getAttr<AttrType::description>().output;
    if(results == nullptr || results->empty())
    {
        return "-";
    }
    switch(getDisplayType(acsr))
    {
        case DisplayType::markers:
        {
            return Tools::getMarkerText(*results, output, time, timeEpsilon);
        }
            break;
        case DisplayType::segments:
        {
            return Tools::getSegmentText(*results, output, time);
        }
            break;
        case DisplayType::grid:
        {
            auto const hasBinName = bin < output.binNames.size() && !output.binNames[bin].empty();
            auto const binName = "bin" + juce::String(bin) + (hasBinName ? ("-" + output.binNames[bin]) : "");
            return "(" + binName + ") " +  Tools::getGridText(*results, output, time, bin);
        }
            break;
    }
    anlStrongAssert(false);
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
        return !v.values.empty();
    });
    if(it == results->cend())
    {
        return std::optional<Zoom::Range>();
    }
    auto const [min, max] = std::minmax_element(it->values.cbegin(), it->values.cend());
    if(it->values.size() == 1)
    {
        return Zoom::Range{static_cast<double>(*min), static_cast<double>(std::max(*max, *min + std::numeric_limits<float>::epsilon()))};
    }
    auto const firstRange = Zoom::Range{static_cast<double>(*min), static_cast<double>(*max)};
    return std::accumulate(results->cbegin() + 1, results->cend(), firstRange, [](auto const r, auto const& v)
    {
        if(v.values.empty())
        {
            return r;
        }
        auto const [min, max] = std::minmax_element(v.values.cbegin(), v.values.cend());
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
