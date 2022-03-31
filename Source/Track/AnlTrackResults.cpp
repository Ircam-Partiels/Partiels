#include "AnlTrackResults.h"

ANALYSE_FILE_BEGIN

static size_t getColumnsNumBins(Track::Results::SharedColumns const& results)
{
    if(results == nullptr)
    {
        return 0_z;
    }
    return std::accumulate(results->cbegin(), results->cend(), 0_z, [](auto const& val, auto const& channel)
                           {
                               return std::accumulate(channel.cbegin(), channel.cend(), val, [](auto const& rval, auto const& column)
                                                      {
                                                          return std::max(rval, std::get<2>(column).size());
                                                      });
                           });
}

static Zoom::Range getColumnsValueRange(Track::Results::SharedColumns const& results)
{
    if(results == nullptr)
    {
        return {};
    }
    auto const resultRange = std::accumulate(results->cbegin(), results->cend(), std::optional<Zoom::Range>{}, [&](auto const& range, auto const& channel) -> std::optional<Zoom::Range>
                                             {
                                                 auto const newRange = std::accumulate(channel.cbegin(), channel.cend(), range, [&](auto const& crange, auto const& column) -> std::optional<Zoom::Range>
                                                                                       {
                                                                                           auto const& values = std::get<2>(column);
                                                                                           auto const [min, max] = std::minmax_element(values.cbegin(), values.cend());
                                                                                           if(min == values.cend() || max == values.cend())
                                                                                           {
                                                                                               return crange;
                                                                                           }
                                                                                           anlWeakAssert(std::isfinite(*min) && std::isfinite(*max) && !std::isnan(*min) && !std::isnan(*max));
                                                                                           if(!std::isfinite(*min) || !std::isfinite(*max) || std::isnan(*min) || std::isnan(*max))
                                                                                           {
                                                                                               return crange;
                                                                                           }
                                                                                           Zoom::Range const newCrange{*min, *max};
                                                                                           return crange.has_value() ? newCrange.getUnionWith(*crange) : newCrange;
                                                                                       });
                                                 return range.has_value() ? (newRange.has_value() ? newRange->getUnionWith(*range) : range) : newRange;
                                             });
    if(resultRange.has_value())
    {
        return *resultRange;
    }
    return {};
}

static Zoom::Range getPointsValueRange(Track::Results::SharedPoints const& results)
{
    if(results == nullptr)
    {
        return {};
    }
    bool initialized = false;
    return std::accumulate(results->cbegin(), results->cend(), Zoom::Range(0.0, 0.0), [&](auto const& range, auto const& channel)
                           {
                               auto const [min, max] = std::minmax_element(channel.cbegin(), channel.cend(), [](auto const& lhs, auto const& rhs)
                                                                           {
                                                                               if(!std::get<2>(rhs).has_value())
                                                                               {
                                                                                   return false;
                                                                               }
                                                                               if(!std::get<2>(lhs).has_value())
                                                                               {
                                                                                   return true;
                                                                               }
                                                                               return *std::get<2>(lhs) < *std::get<2>(rhs);
                                                                           });
                               if(min == channel.cend() || max == channel.cend() || !std::get<2>(*min).has_value() || !std::get<2>(*max).has_value())
                               {
                                   return range;
                               }
                               anlWeakAssert(std::isfinite(*std::get<2>(*min)) && std::isfinite(*std::get<2>(*max)) && !std::isnan(*std::get<2>(*min)) && !std::isnan(*std::get<2>(*max)));
                               if(!std::isfinite(*std::get<2>(*min)) || !std::isfinite(*std::get<2>(*max)) || std::isnan(*std::get<2>(*min)) || std::isnan(*std::get<2>(*max)))
                               {
                                   return range;
                               }
                               Zoom::Range const newRange{*std::get<2>(*min), *std::get<2>(*max)};
                               return std::exchange(initialized, true) ? range.getUnionWith(newRange) : newRange;
                           });
}

Track::Results Track::Results::create(std::variant<SharedMarkers, SharedPoints, SharedColumns> results)
{
    if(std::get_if<SharedMarkers>(&results) != nullptr)
    {
        return Results(std::move(results), 0_z, {});
    }
    if(auto const* pointsPtr = std::get_if<SharedPoints>(&results))
    {
        auto const range = getPointsValueRange(*pointsPtr);
        return Results(std::move(results), 1_z, range);
    }
    if(auto const* columnsPtr = std::get_if<SharedColumns>(&results))
    {
        auto const numBins = getColumnsNumBins(*columnsPtr);
        auto const range = getColumnsValueRange(*columnsPtr);
        return Results(std::move(results), numBins, range);
    }
    return {};
}

Track::Results::Results(std::variant<SharedMarkers, SharedPoints, SharedColumns> results, size_t const numBins, Zoom::Range const& valueRange)
: mResults(std::move(results))
, mNumBins(numBins)
, mValueRange(valueRange)
{
}

Track::Results::SharedMarkers const Track::Results::getMarkers() const noexcept
{
    if(auto const* markersPtr = std::get_if<SharedMarkers>(&mResults))
    {
        return *markersPtr;
    }
    return nullptr;
}

Track::Results::SharedPoints const Track::Results::getPoints() const noexcept
{
    if(auto const* pointsPtr = std::get_if<SharedPoints>(&mResults))
    {
        return *pointsPtr;
    }
    return nullptr;
}

Track::Results::SharedColumns const Track::Results::getColumns() const noexcept
{
    if(auto const* columnsPtr = std::get_if<SharedColumns>(&mResults))
    {
        return *columnsPtr;
    }
    return nullptr;
}

size_t const Track::Results::getNumChannels() const noexcept
{
    if(auto const markers = getMarkers())
    {
        return markers->size();
    }
    else if(auto const points = getPoints())
    {
        return points->size();
    }
    else if(auto const columns = getColumns())
    {
        return columns->size();
    }
    return 0_z;
}

size_t const Track::Results::getNumBins() const noexcept
{
    return mNumBins;
}

Zoom::Range const& Track::Results::getValueRange() const noexcept
{
    return mValueRange;
}

bool Track::Results::isEmpty() const noexcept
{
    if(auto const markers = getMarkers())
    {
        return markers == nullptr || markers->empty() || std::all_of(markers->cbegin(), markers->cend(), [](auto const& channel)
                                                                     {
                                                                         return channel.empty();
                                                                     });
    }
    else if(auto const points = getPoints())
    {
        return points == nullptr || points->empty() || std::all_of(points->cbegin(), points->cend(), [](auto const& channel)
                                                                   {
                                                                       return channel.empty();
                                                                   });
    }
    else if(auto const columns = getColumns())
    {
        return columns == nullptr || columns->empty() || std::all_of(columns->cbegin(), columns->cend(), [](auto const& channel)
                                                                     {
                                                                         return channel.empty();
                                                                     });
    }
    return true;
}

bool Track::Results::operator==(Results const& rhd) const noexcept
{
    return getMarkers() == rhd.getMarkers() &&
           getPoints() == rhd.getPoints() &&
           getColumns() == rhd.getColumns();
}

bool Track::Results::operator!=(Results const& rhd) const noexcept
{
    return !(*this == rhd);
}

std::optional<std::string> Track::Results::getValue(SharedMarkers results, size_t channel, Zoom::Range const& globalRange, double time)
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

std::optional<float> Track::Results::getValue(SharedPoints results, size_t channel, Zoom::Range const& globalRange, double time)
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
    if((next - end) > std::numeric_limits<double>::epsilon() || !std::get<2>(*first).has_value())
    {
        return std::get<2>(*second);
    }
    auto const ratio = std::max(std::min((time - end) / (next - end), 1.0), 0.0);
    if(std::isnan(ratio) || !std::isfinite(ratio)) // Extra check in case (next - end) < std::numeric_limits<double>::epsilon()
    {
        return std::get<2>(*second);
    }
    return (1.0 - ratio) * *std::get<2>(*first) + ratio * *std::get<2>(*second);
}

std::optional<float> Track::Results::getValue(SharedColumns results, size_t channel, Zoom::Range const& globalRange, double time, size_t bin)
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
    if(bin >= column.size())
    {
        return {};
    }
    return column[bin];
}

template <typename T>
typename T::const_iterator findFirstIteratorAt(T const& results, Zoom::Range const& globalRange, double time)
{
    anlWeakAssert(!globalRange.isEmpty());
    if(globalRange.isEmpty())
    {
        return results.cend();
    }
    if(results.empty())
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
        auto it = std::find_if(std::make_reverse_iterator(expectedIt), results.crend(), [&](auto const& result)
                               {
                                   return std::get<0>(result) + std::get<1>(result) < time;
                               });
        if(it == results.crend())
        {
            return results.cbegin();
        }
        return std::next(it).base();
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

Track::Results::Markers::const_iterator Track::Results::findFirstAt(Markers const& results, Zoom::Range const& globalRange, double time)
{
    return findFirstIteratorAt(results, globalRange, time);
}

Track::Results::Points::const_iterator Track::Results::findFirstAt(Points const& results, Zoom::Range const& globalRange, double time)
{
    return findFirstIteratorAt(results, globalRange, time);
}

Track::Results::Columns::const_iterator Track::Results::findFirstAt(Columns const& results, Zoom::Range const& globalRange, double time)
{
    return findFirstIteratorAt(results, globalRange, time);
}

ANALYSE_FILE_END
