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

std::set<size_t> Track::Result::Modifier::getIndices(CopiedData const& data)
{
    auto const generateIndices = [&](auto const& results)
    {
        std::set<size_t> indices;
        for(auto const& result : results)
        {
            indices.insert(result.first);
        }
        return indices;
    };

    if(auto const* markersData = std::get_if<std::map<size_t, Data::Marker>>(&data))
    {
        return generateIndices(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::map<size_t, Data::Point>>(&data))
    {
        return generateIndices(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::map<size_t, Data::Column>>(&data))
    {
        return generateIndices(*columnsData);
    }
    else
    {
        return {};
    }
}

juce::Range<double> Track::Result::Modifier::getTimeRange(CopiedData const& data)
{
    auto const getRange = [&](auto const& results) -> juce::Range<double>
    {
        if(results.empty())
        {
            return {};
        }
        return {std::get<0_z>(results.cbegin()->second), std::get<0_z>(results.crbegin()->second)};
    };

    if(auto const* markersData = std::get_if<std::map<size_t, Data::Marker>>(&data))
    {
        return getRange(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::map<size_t, Data::Point>>(&data))
    {
        return getRange(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::map<size_t, Data::Column>>(&data))
    {
        return getRange(*columnsData);
    }
    else
    {
        return {};
    }
}

Track::Result::Modifier::CopiedData Track::Result::Modifier::duplicateFrames(CopiedData const& data, juce::Range<double> const& range, size_t destinationIndex, double destinationTime)
{
    auto copyFrames = [&](auto const& results) -> CopiedData
    {
        if(results.empty())
        {
            return {};
        }
        using result_type = typename std::remove_const<typename std::remove_reference<decltype(results)>::type>::type;
        auto const indexDifference = static_cast<long>(destinationIndex) - static_cast<long>(results.cbegin()->first);
        auto const timeDifference = destinationTime - range.getStart();
        result_type map;
        for(auto const& pair : results)
        {
            auto frame = pair.second;
            std::get<0_z>(frame) += timeDifference;
            auto const index = static_cast<size_t>(static_cast<long>(pair.first) + indexDifference);
            map[index] = std::move(frame);
        }
        return map;
    };

    if(auto const* markersData = std::get_if<std::map<size_t, Data::Marker>>(&data))
    {
        return copyFrames(*markersData);
    }
    else if(auto const* pointsData = std::get_if<std::map<size_t, Data::Point>>(&data))
    {
        return copyFrames(*pointsData);
    }
    else if(auto const* columnsData = std::get_if<std::map<size_t, Data::Column>>(&data))
    {
        return copyFrames(*columnsData);
    }
    return {};
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

std::optional<size_t> Track::Result::Modifier::getIndex(Accessor const& accessor, size_t const channel, double time)
{
    auto const findIndex = [&](auto const& results) -> std::optional<size_t>
    {
        if(channel >= results.size())
        {
            return {};
        }
        auto& channelFrames = results.at(channel);
        auto start = Result::Data::findFirstAt(channelFrames, time);
        if(start == channelFrames.cend())
        {
            return channelFrames.size();
        }
        return static_cast<size_t>(std::distance(channelFrames.cbegin(), std::get<0_z>(*start) < time ? std::next(start) : start));
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
        return findIndex(*markers);
    }
    if(auto points = results.getPoints())
    {
        return findIndex(*points);
    }
    if(auto columns = results.getColumns())
    {
        return findIndex(*columns);
    }
    return {};
}

std::set<size_t> Track::Result::Modifier::getIndices(Accessor const& accessor, size_t const channel, juce::Range<double> const& range)
{
    auto const generateIndices = [&](auto const& results) -> std::set<size_t>
    {
        if(channel >= results.size())
        {
            return {};
        }
        auto& channelFrames = results.at(channel);
        auto start = Result::Data::findFirstAt(channelFrames, range.getStart());
        if(start != channelFrames.cend() && std::get<0_z>(*start) < range.getStart())
        {
            start = std::next(start);
        }
        if(start == channelFrames.cend())
        {
            return {};
        }
        std::set<size_t> indices;
        auto position = static_cast<size_t>(std::distance(channelFrames.cbegin(), start));
        auto end = Result::Data::findFirstAt(channelFrames, range.getEnd());
        if(end != channelFrames.cend() && std::get<0_z>(*end) < range.getEnd())
        {
            end = std::next(end);
        }
        while(start != end)
        {
            indices.insert(position++);
            start = std::next(start);
        }
        return indices;
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
        return generateIndices(*markers);
    }
    if(auto points = results.getPoints())
    {
        return generateIndices(*points);
    }
    if(auto columns = results.getColumns())
    {
        return generateIndices(*columns);
    }
    return {};
}

Track::Result::Modifier::CopiedData Track::Result::Modifier::copyFrames(Accessor const& accessor, size_t const channel, std::set<size_t> const& indices)
{
    auto const getData = [&](auto& results)
    {
        using result_type = typename std::remove_reference<decltype(results)>::type::value_type;
        using data_type = typename result_type::value_type;
        std::map<size_t, data_type> map;
        if(channel >= results.size())
        {
            return map;
        }
        auto const& channelFrames = results[channel];
        for(auto it = indices.cbegin(); it != indices.cend(); ++it)
        {
            if(*it < channelFrames.size())
            {
                map[*it] = channelFrames[*it];
            }
        }
        return map;
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
        return getData(*markers);
    }
    if(auto points = results.getPoints())
    {
        return getData(*points);
    }
    if(auto columns = results.getColumns())
    {
        return getData(*columns);
    }
    return {};
}

bool Track::Result::Modifier::insertFrames(Accessor& accessor, size_t const channel, CopiedData const& data, juce::String const& commit)
{
    auto const applyChange = [&](auto& results, auto const& newData)
    {
        if(channel >= results.size())
        {
            return false;
        }
        auto& channelFrames = results[channel];
        for(auto it = newData.cbegin(); it != newData.cend(); ++it)
        {
            auto constexpr maxTime = std::numeric_limits<double>::max();
            auto const index = it->first;
            auto const previousTime = index > 0_z && index - 1_z < channelFrames.size() ? std::get<0_z>(channelFrames[index - 1_z]) + std::get<1_z>(channelFrames[index - 1_z]) : 0.0;
            auto const nextTime = index < channelFrames.size() ? std::get<0_z>(channelFrames[index]) : maxTime;
            MiscWeakAssert(nextTime >= previousTime);
            auto value = it->second;
            std::get<0_z>(value) = std::clamp(std::get<0_z>(value), previousTime, std::max(nextTime, previousTime));
            std::get<1_z>(value) = std::min(std::get<1_z>(value), nextTime - std::get<0_z>(value));
            channelFrames.insert(channelFrames.begin() + static_cast<long>(index), value);
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
            if(auto const* markersData = std::get_if<std::map<size_t, Data::Marker>>(&data))
            {
                return applyChange(*markers, *markersData);
            }
            return false;
        }
        if(auto points = results.getPoints())
        {
            if(auto const* pointsData = std::get_if<std::map<size_t, Data::Point>>(&data))
            {
                return applyChange(*points, *pointsData);
            }
            return false;
        }
        if(auto columns = results.getColumns())
        {
            if(auto const* columnsData = std::get_if<std::map<size_t, Data::Column>>(&data))
            {
                return applyChange(*columns, *columnsData);
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

bool Track::Result::Modifier::eraseFrames(Accessor& accessor, size_t const channel, std::set<size_t> const& indices, juce::String const& commit)
{
    if(indices.empty())
    {
        return true;
    }

    auto const applyChange = [&](auto& results)
    {
        if(channel >= results.size())
        {
            return false;
        }
        auto& channelFrames = results[channel];
        auto changed = false;
        for(auto it = indices.crbegin(); it != indices.crend(); ++it)
        {
            auto const index = *it;
            if(index < channelFrames.size())
            {
                channelFrames.erase(channelFrames.begin() + static_cast<long>(index));
                changed = true;
            }
        }
        return changed;
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
            return applyChange(*markers);
        }
        if(auto points = results.getPoints())
        {
            return applyChange(*points);
        }
        if(auto columns = results.getColumns())
        {
            return applyChange(*columns);
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
, mSavedSelection(selection)
, mSavedData(copyFrames(mGetAccessorFn(), channel, getIndices(mGetAccessorFn(), channel, selection)))
{
}

bool Track::Result::Modifier::ActionErase::perform()
{
    if(eraseFrames(mGetAccessorFn(), mChannel, getIndices(mSavedData), mNewCommit))
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

Track::Result::Modifier::ActionPaste::ActionPaste(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection, CopiedData const& data, double destination)
: ActionBase(fn, channel)
, mDestinationSelection(selection.movedToStartAt(destination))
, mSavedData(copyFrames(mGetAccessorFn(), channel, getIndices(mGetAccessorFn(), channel, mDestinationSelection)))
, mCopyIndex(getIndex(mGetAccessorFn(), channel, mDestinationSelection.getStart()))
, mCopiedData(duplicateFrames(data, selection, mCopyIndex.has_value() ? *mCopyIndex : 0_z, mDestinationSelection.getStart()))
{
}

bool Track::Result::Modifier::ActionPaste::perform()
{
    auto& accessor = mGetAccessorFn();
    if(eraseFrames(accessor, mChannel, getIndices(mSavedData), mNewCommit))
    {
        if(insertFrames(accessor, mChannel, mCopiedData, mNewCommit))
        {
            return true;
        }
        else
        {
            if(!insertFrames(accessor, mChannel, mSavedData, mCurrentCommit))
            {
                MiscWeakAssert(false);
            }
        }
    }
    return false;
}

bool Track::Result::Modifier::ActionPaste::undo()
{
    auto& accessor = mGetAccessorFn();
    if(eraseFrames(accessor, mChannel, getIndices(mCopiedData), mCurrentCommit))
    {
        if(insertFrames(accessor, mChannel, mSavedData, mCurrentCommit))
        {
            return true;
        }
        else
        {
            MiscWeakAssert(false);
            if(!insertFrames(accessor, mChannel, mCopiedData, mNewCommit))
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
