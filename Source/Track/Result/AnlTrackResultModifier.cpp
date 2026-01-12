#include "AnlTrackResultModifier.h"
#include "../AnlTrackTools.h"

ANALYSE_FILE_BEGIN

namespace
{
    void showAccessWarning()
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Results cannot be accessed!"))
                                 .withMessage(juce::translate("The results are already used by another process and cannot be modified. Please, wait for a valid access before modifying results."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
} // namespace

juce::Range<double> Track::Result::Modifier::getTimeRange(ChannelData const& data)
{
    auto const getRange = [&](auto const& results) -> juce::Range<double>
    {
        if(results.empty())
        {
            return {};
        }
        return {std::get<0_z>(results.front()), std::get<0_z>(results.back())};
    };

    if(auto const* markersData = std::get_if<std::vector<Data::Marker>>(&data))
    {
        return getRange(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::vector<Data::Point>>(&data))
    {
        return getRange(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::vector<Data::Column>>(&data))
    {
        return getRange(*columnsData);
    }
    else
    {
        return {};
    }
}

std::optional<double> Track::Result::Modifier::getTime(Accessor const& accessor, size_t const channel, size_t const index)
{
    auto const findTime = [&](auto const& result) -> std::optional<double>
    {
        if(channel >= result.size())
        {
            return {};
        }
        auto const& channelResults = result.at(channel);
        if(index >= channelResults.size())
        {
            return {};
        }
        return std::get<0_z>(channelResults.at(index));
    };

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return {};
    }
    if(auto const markers = results.getMarkers())
    {
        return findTime(*markers);
    }
    if(auto const points = results.getPoints())
    {
        return findTime(*points);
    }
    if(auto const columns = results.getColumns())
    {
        return findTime(*columns);
    }
    return {};
}

bool Track::Result::Modifier::isEmpty(ChannelData const& data)
{
    auto const doIsEmpty = [&](auto const& results) -> bool
    {
        return results.empty();
    };

    if(auto const* markersData = std::get_if<std::vector<Data::Marker>>(&data))
    {
        return doIsEmpty(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::vector<Data::Point>>(&data))
    {
        return doIsEmpty(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::vector<Data::Column>>(&data))
    {
        return doIsEmpty(*columnsData);
    }
    return true;
}

bool Track::Result::Modifier::matchFrame(Accessor const& accessor, size_t const channel, double const time, double const epsilon)
{
    auto const doMatch = [&](auto const& results)
    {
        using result_type = typename std::remove_const<typename std::remove_reference<decltype(results)>::type>::type;
        using data_type = typename result_type::value_type::value_type;
        if(channel >= results.size())
        {
            return false;
        }
        auto const& channelFrames = results.at(channel);
        auto const start = std::lower_bound(channelFrames.cbegin(), channelFrames.cend(), time - epsilon, Result::lower_cmp<data_type>);
        return start != channelFrames.cend() && std::get<0_z>(*start) <= time + epsilon;
    };

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return false;
    }
    if(auto markers = results.getMarkers())
    {
        return doMatch(*markers);
    }
    if(auto points = results.getPoints())
    {
        return doMatch(*points);
    }
    if(auto columns = results.getColumns())
    {
        return doMatch(*columns);
    }
    return false;
}

bool Track::Result::Modifier::canBreak(Accessor const& accessor, size_t const channel, double const time)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return false;
    }
    if(auto points = results.getPoints())
    {
        if(channel >= points->size())
        {
            return false;
        }
        auto const& channelFrames = points->at(channel);
        auto const start = std::lower_bound(channelFrames.cbegin(), channelFrames.cend(), time, Result::lower_cmp<Results::Point>);
        return start == channelFrames.cend() || std::get<0_z>(*start) > time || !std::get<2_z>(*start).has_value();
    }
    return false;
}

bool Track::Result::Modifier::containFrames(Accessor const& accessor, size_t const channel, juce::Range<double> const& range)
{
    auto const doContain = [&](auto const& results)
    {
        using result_type = typename std::remove_const<typename std::remove_reference<decltype(results)>::type>::type;
        using data_type = typename result_type::value_type::value_type;
        if(channel >= results.size())
        {
            return false;
        }
        auto const& channelFrames = results.at(channel);
        auto const start = std::lower_bound(channelFrames.cbegin(), channelFrames.cend(), range.getStart(), Result::lower_cmp<data_type>);
        if(range.isEmpty())
        {
            return start != channelFrames.cend() && std::get<0_z>(*start) <= range.getStart();
        }
        return start != channelFrames.cend() && std::get<0_z>(*start) < range.getEnd();
    };

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return false;
    }
    if(auto markers = results.getMarkers())
    {
        return doContain(*markers);
    }
    if(auto points = results.getPoints())
    {
        return doContain(*points);
    }
    if(auto columns = results.getColumns())
    {
        return doContain(*columns);
    }
    return false;
}

Track::Result::ChannelData Track::Result::Modifier::createFrame(Accessor const& accessor, size_t const channel, double const time)
{
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return {};
    }
    if(auto markers = results.getMarkers())
    {
        std::vector<float> extras;
        auto const& extraOutputs = accessor.getAttr<AttrType::description>().extraOutputs;
        for(auto index = 0_z; index < extraOutputs.size(); ++index)
        {
            auto const range = Tools::getExtraRange(accessor, index);
            extras.push_back(range.has_value() ? static_cast<float>(range->getEnd()) : 1.0f);
        }
        return std::vector<Data::Marker>{{time, 0.0, {}, extras}};
    }
    if(auto points = results.getPoints())
    {
        std::vector<float> extras;
        auto const& extraOutputs = accessor.getAttr<AttrType::description>().extraOutputs;
        for(auto index = 0_z; index < extraOutputs.size(); ++index)
        {
            auto const range = Tools::getExtraRange(accessor, index);
            extras.push_back(range.has_value() ? static_cast<float>(range->getEnd()) : 1.0f);
        }
        return std::vector<Data::Point>{{time, 0.0, Data::getValue(points, channel, time), extras}};
    }
    return {};
}

Track::Result::ChannelData Track::Result::Modifier::copyFrames(Accessor const& accessor, size_t const channel, juce::Range<double> const& range)
{
    auto const doCopy = [&](auto const& results)
    {
        using result_type = typename std::remove_const<typename std::remove_reference<decltype(results)>::type>::type;
        using data_type = typename result_type::value_type::value_type;
        std::vector<data_type> copy;
        if(channel >= results.size())
        {
            return copy;
        }
        auto const& channelFrames = results.at(channel);
        auto start = std::lower_bound(channelFrames.cbegin(), channelFrames.cend(), range.getStart(), Result::lower_cmp<data_type>);
        auto const end = std::upper_bound(start, channelFrames.cend(), range.getEnd(), Result::upper_cmp<data_type>);
        copy.reserve(static_cast<size_t>(std::distance(start, end)));
        if(range.isEmpty())
        {
            if(start != channelFrames.cend() && std::get<0_z>(*start) <= range.getStart())
            {
                copy.push_back(*start);
            }
        }
        else
        {
            while(start != channelFrames.cend() && std::get<0_z>(*start) < range.getEnd())
            {
                copy.push_back(*start++);
            }
        }
        return copy;
    };

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        showAccessWarning();
        return {};
    }
    if(auto markers = results.getMarkers())
    {
        return doCopy(*markers);
    }
    if(auto points = results.getPoints())
    {
        return doCopy(*points);
    }
    if(auto columns = results.getColumns())
    {
        return doCopy(*columns);
    }
    return {};
}

Track::Result::ChannelData Track::Result::Modifier::duplicateFrames(ChannelData const& data, double const destinationTime)
{
    auto const doDuplicate = [&](auto const& results) -> ChannelData
    {
        if(results.empty())
        {
            return {};
        }
        auto const timeDifference = destinationTime - std::get<0_z>(results.front());
        auto copy = results;
        for(auto& frame : copy)
        {
            std::get<0_z>(frame) += timeDifference;
        }
        return copy;
    };

    if(auto const* markersData = std::get_if<std::vector<Data::Marker>>(&data))
    {
        return doDuplicate(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::vector<Data::Point>>(&data))
    {
        return doDuplicate(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::vector<Data::Column>>(&data))
    {
        return doDuplicate(*columnsData);
    }
    return {};
}

bool Track::Result::Modifier::eraseFrames(Accessor& accessor, size_t const channel, juce::Range<double> const& range, bool preserveFullDuration, double endTime, juce::String const& commit)
{
    auto const doErase = [&](auto& results)
    {
        using result_type = typename std::remove_reference<decltype(results)>::type;
        using data_type = typename result_type::value_type::value_type;
        if(channel >= results.size())
        {
            return false;
        }

        auto& channelFrames = results[channel];
        auto const erase = [&](auto const first, auto const last)
        {
            // If the preserveFullDuration is true, we need to extend the previous frame duration to fill the gap
            if(preserveFullDuration && first != channelFrames.begin() && first != channelFrames.end())
            {
                auto const previous = std::prev(first);
                auto const previousEnd = std::get<0_z>(*previous) + std::get<1_z>(*previous);
                auto const nextStart = last != channelFrames.end() ? std::get<0_z>(*last) : endTime;
                if(previousEnd >= std::get<0_z>(*first)) // Check if the previous frame overlaps or reaches the erased range
                {
                    std::get<1_z>(*previous) = nextStart - std::get<0_z>(*previous);
                }
            }
            channelFrames.erase(first, last);
        };

        auto const start = std::lower_bound(channelFrames.begin(), channelFrames.end(), range.getStart(), Result::lower_cmp<data_type>);
        if(range.isEmpty())
        {
            if(start != channelFrames.end() && std::get<0_z>(*start) <= range.getStart())
            {
                erase(start, std::next(start));
                return true;
            }
            return false;
        }
        auto const end = std::upper_bound(start, channelFrames.end(), range.getEnd(), Result::upper_cmp<data_type>);
        erase(start, end);
        return true;
    };

    auto const getAccessAndParse = [&](auto& results)
    {
        auto const access = results.getWriteAccess();
        if(!static_cast<bool>(access))
        {
            showAccessWarning();
            return false;
        }
        if(auto markers = results.getMarkers())
        {
            return doErase(*markers);
        }
        if(auto points = results.getPoints())
        {
            return doErase(*points);
        }
        if(auto columns = results.getColumns())
        {
            return doErase(*columns);
        }
        return false;
    };

    auto results(accessor.getAttr<AttrType::results>());
    if(getAccessAndParse(results))
    {
        auto file = accessor.getAttr<AttrType::file>();
        file.commit = commit;
        accessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        accessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        return true;
    }
    return false;
}

bool Track::Result::Modifier::insertFrames(Accessor& accessor, size_t const channel, ChannelData const& data, bool preserveFullDuration, double endTime, juce::String const& commit)
{
    auto const range = getTimeRange(data);
    auto const doInsert = [&](auto& results, auto const& newData)
    {
        if(newData.empty())
        {
            return true;
        }
        using result_type = typename std::remove_reference<decltype(results)>::type;
        using data_type = typename result_type::value_type::value_type;
        if(channel >= results.size())
        {
            return false;
        }
        auto& channelFrames = results[channel];
        // Erase the range where the data will be inserted
        auto start = std::lower_bound(channelFrames.begin(), channelFrames.end(), range.getStart(), Result::lower_cmp<data_type>);
        auto const end = std::upper_bound(start, channelFrames.end(), range.getEnd(), Result::upper_cmp<data_type>);
        auto const extendEnd = [&]()
        {
            if(preserveFullDuration && end != channelFrames.begin())
            {
                auto const previous = std::prev(end);
                auto const previousEnd = std::get<0_z>(*previous) + std::get<1_z>(*previous);
                auto const nextStart = end != channelFrames.end() ? std::get<0_z>(*end) : endTime;
                return previousEnd >= nextStart;
            }
            return false;
        }();
        start = channelFrames.erase(start, end);
        // Insert the new frames
        auto const first = channelFrames.insert(start, newData.cbegin(), newData.cend());
        auto const last = std::next(first, static_cast<long>(newData.size()));
        // Sanitize the duration of the frames
        auto const sanitizeDuration = [&](auto const iterator, bool forceEnd)
        {
            if(iterator != channelFrames.begin() && iterator != channelFrames.end())
            {
                auto previous = std::prev(iterator);
                auto const previousDuration = std::get<1_z>(*previous);
                auto const maxPreviousDuration = std::get<0_z>(*iterator) - std::get<0_z>(*previous);
                std::get<1_z>(*previous) = forceEnd ? maxPreviousDuration : std::min(previousDuration, maxPreviousDuration);
            }
        };
        sanitizeDuration(first, false);
        sanitizeDuration(last, extendEnd);
        return true;
    };

    auto const getAccessAndParse = [&](auto& results)
    {
        auto const access = results.getWriteAccess();
        if(!static_cast<bool>(access))
        {
            showAccessWarning();
            return false;
        }
        if(auto markers = results.getMarkers())
        {
            if(auto const* markersData = std::get_if<std::vector<Data::Marker>>(&data))
            {
                return doInsert(*markers, *markersData);
            }
            return false;
        }
        if(auto points = results.getPoints())
        {
            if(auto const* pointsData = std::get_if<std::vector<Data::Point>>(&data))
            {
                return doInsert(*points, *pointsData);
            }
            return false;
        }
        if(auto columns = results.getColumns())
        {
            if(auto const* columnsData = std::get_if<std::vector<Data::Column>>(&data))
            {
                return doInsert(*columns, *columnsData);
            }
            return false;
        }
        return false;
    };

    auto results(accessor.getAttr<AttrType::results>());
    if(getAccessAndParse(results))
    {
        auto file = accessor.getAttr<AttrType::file>();
        file.commit = commit;
        accessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        accessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        return true;
    }
    return false;
}

bool Track::Result::Modifier::resetFrameDurations(Accessor& accessor, size_t const channel, juce::Range<double> const& range, DurationResetMode const mode, double endTime, juce::String const& commit)
{
    auto const doReset = [&](auto& results)
    {
        using result_type = typename std::remove_reference<decltype(results)>::type;
        using data_type = typename result_type::value_type::value_type;
        if(channel >= results.size())
        {
            return false;
        }
        auto& channelFrames = results[channel];
        auto const start = std::lower_bound(channelFrames.begin(), channelFrames.end(), range.getStart(), Result::lower_cmp<data_type>);
        auto const end = std::upper_bound(start, channelFrames.end(), range.getEnd(), Result::upper_cmp<data_type>);
        for(auto it = start; it != end; ++it)
        {
            if(mode == DurationResetMode::toZero)
            {
                std::get<1_z>(*it) = 0.0;
            }
            else if(mode == DurationResetMode::toFull)
            {
                auto const nextEnd = (std::next(it) != channelFrames.end()) ? std::get<0_z>(*std::next(it)) : endTime;
                std::get<1_z>(*it) = nextEnd - std::get<0_z>(*it);
            }
        }
        return true;
    };

    auto const getAccessAndParse = [&](auto& results)
    {
        auto const access = results.getWriteAccess();
        if(!static_cast<bool>(access))
        {
            showAccessWarning();
            return false;
        }
        if(auto markers = results.getMarkers())
        {
            return doReset(*markers);
        }
        if(auto points = results.getPoints())
        {
            return doReset(*points);
        }
        if(auto columns = results.getColumns())
        {
            return doReset(*columns);
        }
        return false;
    };

    auto results(accessor.getAttr<AttrType::results>());
    if(getAccessAndParse(results))
    {
        auto file = accessor.getAttr<AttrType::file>();
        file.commit = commit;
        accessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
        accessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        return true;
    }
    return false;
}

Track::Result::Modifier::ActionBase::ActionBase(std::function<Accessor&()> fn, size_t const channel)
: mGetAccessorFn(fn)
, mChannel(channel)
, mCurrentCommit(mGetAccessorFn().getAttr<AttrType::file>().commit)
, mNewCommit(juce::Uuid().toString())
{
}

Track::Result::Modifier::ActionErase::ActionErase(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection, bool preserveFullDuration, double endTime)
: ActionBase(fn, channel)
, mSavedData(copyFrames(mGetAccessorFn(), channel, selection))
, mPreserveFullDuration(preserveFullDuration)
, mEndTime(endTime)
{
}

bool Track::Result::Modifier::ActionErase::perform()
{
    if(eraseFrames(mGetAccessorFn(), mChannel, getTimeRange(mSavedData), mPreserveFullDuration, mEndTime, mNewCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

bool Track::Result::Modifier::ActionErase::undo()
{
    if(insertFrames(mGetAccessorFn(), mChannel, mSavedData, false, 0.0, mCurrentCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

Track::Result::Modifier::ActionPaste::ActionPaste(std::function<Accessor&()> fn, size_t const channel, ChannelData const& data, double destination, bool preserveFullDuration, double endTime)
: ActionBase(fn, channel)
, mSavedData(copyFrames(mGetAccessorFn(), channel, getTimeRange(data).movedToStartAt(destination)))
, mChannelData(duplicateFrames(data, destination))
, mPreserveFullDuration(preserveFullDuration)
, mEndTime(endTime)
{
}

bool Track::Result::Modifier::ActionPaste::perform()
{
    auto& accessor = mGetAccessorFn();
    if(insertFrames(accessor, mChannel, mChannelData, mPreserveFullDuration, mEndTime, mNewCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

bool Track::Result::Modifier::ActionPaste::undo()
{
    auto& accessor = mGetAccessorFn();
    if(eraseFrames(accessor, mChannel, getTimeRange(mChannelData), mPreserveFullDuration, mEndTime, mCurrentCommit))
    {
        if(insertFrames(accessor, mChannel, mSavedData, false, 0.0, mCurrentCommit))
        {
            return true;
        }
        else
        {
            MiscWeakAssert(false);
            if(!insertFrames(accessor, mChannel, mChannelData, false, 0.0, mNewCommit))
            {
                MiscWeakAssert(false);
            }
        }
    }
    else
    {
        MiscWeakAssert(false);
    }
    return false;
}

Track::Result::Modifier::ActionResetDuration::ActionResetDuration(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection, DurationResetMode mode, double endTime)
: ActionBase(fn, channel)
, mSavedData(copyFrames(mGetAccessorFn(), channel, selection))
, mResetMode(mode)
, mEndTime(endTime)
{
}

bool Track::Result::Modifier::ActionResetDuration::perform()
{
    if(resetFrameDurations(mGetAccessorFn(), mChannel, getTimeRange(mSavedData), mResetMode, mEndTime, mNewCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

bool Track::Result::Modifier::ActionResetDuration::undo()
{
    if(insertFrames(mGetAccessorFn(), mChannel, mSavedData, false, 0.0, mCurrentCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

Track::Result::Modifier::FocusRestorer::FocusRestorer(std::function<Accessor&()> fn)
: mGetAccessorFn(fn)
, mFocus(mGetAccessorFn().getAttr<AttrType::focused>())
{
}

bool Track::Result::Modifier::FocusRestorer::perform()
{
    mGetAccessorFn().setAttr<AttrType::focused>(mFocus, NotificationType::synchronous);
    return true;
}

bool Track::Result::Modifier::FocusRestorer::undo()
{
    return perform();
}

ANALYSE_FILE_END
