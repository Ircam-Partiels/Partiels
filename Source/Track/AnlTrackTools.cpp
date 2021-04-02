#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

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

juce::String Track::Tools::getMarkerText(std::vector<Plugin::Result> const& results, Plugin::Output const& output, double time)
{
    auto const rt = secondsToRealTime(time);
    auto it = std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
    {
        return result.hasTimestamp && getEndRealTime(result) >= rt;
    });
    if(it != results.cend() && getEndRealTime(*it) <= rt)
    {
        return it->label.empty() ? output.unit : it->label;
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

ANALYSE_FILE_END
