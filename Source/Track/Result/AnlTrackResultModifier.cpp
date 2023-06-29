#include "AnlTrackResultModifier.h"

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

bool Track::Result::Modifier::matchFrame(Accessor const& accessor, size_t const channel, double const time)
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
        auto const start = std::lower_bound(channelFrames.cbegin(), channelFrames.cend(), time, Result::lower_cmp<data_type>);
        return start != channelFrames.cend() && std::get<0_z>(*start) <= time;
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
    return {};
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
        return start != channelFrames.cend() && std::get<0_z>(*start) < range.getEnd();
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
    return {};
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
        return std::vector<Data::Marker>{{time, 0.0, {}}};
    }
    if(auto points = results.getPoints())
    {
        return std::vector<Data::Point>{{time, 0.0, Data::getValue(points, channel, time)}};
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
        while(start != channelFrames.cend() && std::get<0_z>(*start) < range.getEnd())
        {
            copy.push_back(*start++);
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

bool Track::Result::Modifier::eraseFrames(Accessor& accessor, size_t const channel, juce::Range<double> const& range, juce::String const& commit)
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
        auto const start = std::lower_bound(channelFrames.begin(), channelFrames.end(), range.getStart(), Result::lower_cmp<data_type>);
        auto const end = std::upper_bound(start, channelFrames.end(), range.getEnd(), Result::upper_cmp<data_type>);
        channelFrames.erase(start, end);
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

bool Track::Result::Modifier::insertFrames(Accessor& accessor, size_t const channel, ChannelData const& data, juce::String const& commit)
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
        // ensure the frames where the data is copied is empty
        auto start = std::lower_bound(channelFrames.begin(), channelFrames.end(), range.getStart(), Result::lower_cmp<data_type>);
        auto const end = std::upper_bound(start, channelFrames.end(), range.getEnd(), Result::upper_cmp<data_type>);
        start = channelFrames.erase(start, end);
        // inserts the new frames
        auto const it = channelFrames.insert(start, newData.cbegin(), newData.cend());
        // sanitize the duration of the frame before
        auto const sanitizeDuration = [&](auto const iterator)
        {
            if(iterator != channelFrames.begin())
            {
                auto previous = std::prev(iterator);
                std::get<1_z>(*previous) = std::min(std::get<1_z>(*previous), std::get<0_z>(*iterator) - std::get<0_z>(*previous));
            }
        };
        sanitizeDuration(it);
        sanitizeDuration(std::next(it, static_cast<long>(newData.size())));
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

Track::Result::Modifier::ActionBase::ActionBase(std::function<Accessor&()> fn, size_t const channel)
: mGetAccessorFn(fn)
, mChannel(channel)
, mCurrentCommit(mGetAccessorFn().getAttr<AttrType::file>().commit)
, mNewCommit(juce::Uuid().toString())
{
}

Track::Result::Modifier::ActionErase::ActionErase(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection)
: ActionBase(fn, channel)
, mSavedData(copyFrames(mGetAccessorFn(), channel, selection))
{
}

bool Track::Result::Modifier::ActionErase::perform()
{
    if(eraseFrames(mGetAccessorFn(), mChannel, getTimeRange(mSavedData), mNewCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

bool Track::Result::Modifier::ActionErase::undo()
{
    if(insertFrames(mGetAccessorFn(), mChannel, mSavedData, mCurrentCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

Track::Result::Modifier::ActionPaste::ActionPaste(std::function<Accessor&()> fn, size_t const channel, ChannelData const& data, double destination)
: ActionBase(fn, channel)
, mSavedData(copyFrames(mGetAccessorFn(), channel, getTimeRange(data).movedToStartAt(destination)))
, mChannelData(duplicateFrames(data, destination))
{
}

bool Track::Result::Modifier::ActionPaste::perform()
{
    auto& accessor = mGetAccessorFn();
    if(insertFrames(accessor, mChannel, mChannelData, mNewCommit))
    {
        return true;
    }
    MiscWeakAssert(false);
    return false;
}

bool Track::Result::Modifier::ActionPaste::undo()
{
    auto& accessor = mGetAccessorFn();
    if(eraseFrames(accessor, mChannel, getTimeRange(mChannelData), mCurrentCommit))
    {
        if(insertFrames(accessor, mChannel, mSavedData, mCurrentCommit))
        {
            return true;
        }
        else
        {
            MiscWeakAssert(false);
            if(!insertFrames(accessor, mChannel, mChannelData, mNewCommit))
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
