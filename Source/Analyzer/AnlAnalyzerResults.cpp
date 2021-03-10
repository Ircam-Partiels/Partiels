#include "AnlAnalyzerResults.h"

ANALYSE_FILE_BEGIN

Analyzer::Results::Container::const_iterator Analyzer::Results::getResultAt(Container const& results, double time)
{
    if(results.empty())
    {
        return results.cend();
    }
    auto const realTime = Vamp::RealTime::fromSeconds(time);
    if(results.front().hasDuration)
    {
        return std::find_if(results.cbegin(), results.cend(), [&](Plugin::Result const& result)
        {
            anlWeakAssert(result.hasTimestamp);
            return realTime >= result.timestamp && realTime < result.timestamp + result.duration;
        });
    }
    return std::lower_bound(results.cbegin(), results.cend(), realTime, [](auto const& result, auto const& t)
    {
        anlWeakAssert(result.hasTimestamp);
        return result.timestamp < t;
    });
}

Zoom::Range Analyzer::Results::getValueRange(Container const& results)
{
    auto it = std::find_if(results.cbegin(), results.cend(), [](auto const& v)
    {
        return !v.values.empty();
    });
    if(it == results.cend())
    {
        return Zoom::Range{Zoom::lowest(), Zoom::max()};
    }
    auto const [min, max] = std::minmax_element(it->values.cbegin(), it->values.cend());
    return std::accumulate(results.cbegin(), results.cend(), Zoom::Range{static_cast<double>(*min), static_cast<double>(*max)}, [](auto const r, auto const& v)
    {
        if(v.values.empty())
        {
            return r;
        }
        auto const [min, max] = std::minmax_element(v.values.cbegin(), v.values.cend());
        return r.getUnionWith({static_cast<double>(*min), static_cast<double>(*max)});
    });
}

Zoom::Range Analyzer::Results::getBinRange(Container const& results)
{
    if(results.empty())
    {
        return Zoom::Range{0.0, 1.0};
    }
    return std::accumulate(results.cbegin(), results.cend(), Zoom::Range::emptyRange(static_cast<double>(results.front().values.size())), [](auto const r, auto const& v)
    {
        return r.getUnionWith({static_cast<double>(v.values.size()), static_cast<double>(v.values.size())});
    });
}

ANALYSE_FILE_END
