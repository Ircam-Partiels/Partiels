#include "AnlTrackResults.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Results::Results(SharedMarkers ptr)
: mResults(ptr)
, mNumBins(0_z)
, mValueRange(0.0, 0.0)
{
}

Track::Results::Results(SharedPoints ptr, Zoom::Range const& valueRange)
: mResults(ptr)
, mNumBins(1_z)
, mValueRange(valueRange)
{
}

Track::Results::Results(SharedColumns ptr, size_t const numBins, Zoom::Range const& valueRange)
: mResults(ptr)
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
    if(auto const* markersPtr = std::get_if<SharedMarkers>(&mResults))
    {
        auto const markers = *markersPtr;
        return markers == nullptr || markers->empty() || std::all_of(markers->cbegin(), markers->cend(), [](auto const& channel)
                                                                     {
                                                                         return channel.empty();
                                                                     });
    }
    else if(auto const* pointsPtr = std::get_if<SharedPoints>(&mResults))
    {
        auto const points = *pointsPtr;
        return points == nullptr || points->empty() || std::all_of(points->cbegin(), points->cend(), [](auto const& channel)
                                                                   {
                                                                       return channel.empty();
                                                                   });
    }
    else if(auto const* columnsPtr = std::get_if<SharedColumns>(&mResults))
    {
        auto const columns = *columnsPtr;
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

ANALYSE_FILE_END
