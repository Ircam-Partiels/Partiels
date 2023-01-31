#include "AnlDocumentSelection.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

void Document::Selection::clearAll(Accessor& accessor, NotificationType notification)
{
    using FocusInfo = Track::FocusInfo;
    for(auto const& trackAcsr : accessor.getAcsrs<AcsrType::tracks>())
    {
        trackAcsr.get().setAttr<Track::AttrType::focused>(FocusInfo{}, notification);
    }
    for(auto const& groupAcsr : accessor.getAcsrs<AcsrType::groups>())
    {
        groupAcsr.get().setAttr<Group::AttrType::focused>(FocusInfo{}, notification);
    }
}

void Document::Selection::selectItem(Accessor& accessor, Item item, bool deselectOthers, bool flipSelection, NotificationType notification)
{
    selectItems(accessor, item, item, deselectOthers, flipSelection, notification);
}

void Document::Selection::selectItems(Accessor& accessor, Item begin, Item end, bool deselectOthers, bool flipSelection, NotificationType notification)
{
    using FocusInfo = Track::FocusInfo;
    MiscWeakAssert(Tools::hasItem(accessor, begin.identifier) && Tools::hasItem(accessor, end.identifier));
    if(!Tools::hasItem(accessor, begin.identifier) || !Tools::hasItem(accessor, end.identifier))
    {
        return;
    }

    if(begin.identifier == end.identifier)
    {
        if(begin.channel.has_value() != end.channel.has_value())
        {
            begin.channel.reset();
            end.channel.reset();
        }
        else if(begin.channel.has_value() && end.channel.has_value() && *begin.channel > *end.channel)
        {
            std::swap(begin, end);
        }
    }
    else if(Tools::getItemPosition(accessor, begin.identifier) > Tools::getItemPosition(accessor, end.identifier))
    {
        std::swap(begin, end);
    }

    auto inRange = false;
    auto const updateState = [&](FocusInfo& info, juce::String const& identifier, size_t const channel, size_t const maxChannels)
    {
        if(identifier == begin.identifier && channel == begin.channel.value_or(0_z))
        {
            MiscWeakAssert(!inRange);
            inRange = true;
        }
        if(inRange)
        {
            if(flipSelection)
            {
                info.flip(channel);
            }
            else
            {
                info.set(channel, true);
            }
        }
        else if(deselectOthers)
        {
            info.set(channel, false);
        }
        if(identifier == end.identifier && channel == end.channel.value_or(maxChannels))
        {
            MiscWeakAssert(inRange);
            inRange = false;
        }
    };

    for(auto const& groupIdentifier : accessor.getAttr<AttrType::layout>())
    {
        MiscWeakAssert(Tools::hasGroupAcsr(accessor, groupIdentifier));
        if(Tools::hasGroupAcsr(accessor, groupIdentifier))
        {
            auto& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
            auto groupStates = groupAcsr.getAttr<Group::AttrType::focused>();

            auto const groupNumChannels = std::max(Group::Tools::getChannelVisibilityStates(groupAcsr).size(), 1_z);
            auto const groupMinChannels = std::min(groupNumChannels, groupStates.size());

            for(auto channelIndex = 0_z; channelIndex < groupMinChannels; ++channelIndex)
            {
                updateState(groupStates, groupIdentifier, channelIndex, groupMinChannels - 1_z);
            }
            groupAcsr.setAttr<Group::AttrType::focused>(groupStates, notification);

            for(auto const& trackIdentifier : groupAcsr.getAttr<Group::AttrType::layout>())
            {
                auto const trackAcsr = Group::Tools::getTrackAcsr(groupAcsr, trackIdentifier);
                if(trackAcsr.has_value())
                {
                    auto trackStates = trackAcsr->get().getAttr<Track::AttrType::focused>();

                    auto const trackNumChannels = std::max(trackAcsr->get().getAttr<Track::AttrType::channelsLayout>().size(), 1_z);
                    auto const trackMinChannels = std::min(trackNumChannels, trackStates.size());
                    for(auto channelIndex = 0_z; channelIndex < trackMinChannels; ++channelIndex)
                    {
                        updateState(trackStates, trackIdentifier, channelIndex, trackMinChannels - 1_z);
                    }
                    trackAcsr->get().setAttr<Track::AttrType::focused>(trackStates, notification);
                }
            }
        }
    }
}

std::set<juce::String> Document::Selection::getTracks(Accessor const& accessor)
{
    std::set<juce::String> tracks;
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::isSelected(trackAcsr.get()))
        {
            tracks.insert(trackAcsr.get().getAttr<Track::AttrType::identifier>());
        }
    }
    return tracks;
}

std::set<juce::String> Document::Selection::getGroups(Accessor const& accessor)
{
    std::set<juce::String> groups;
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    for(auto const& groupAcsr : groupAcsrs)
    {
        if(Group::Tools::isSelected(groupAcsr.get()))
        {
            groups.insert(groupAcsr.get().getAttr<Group::AttrType::identifier>());
        }
    }
    return groups;
}

std::tuple<std::set<juce::String>, std::set<juce::String>> Document::Selection::getItems(Accessor const& accessor)
{
    auto selectedGroups = getGroups(accessor);
    auto selectedTracks = getTracks(accessor);
    auto it = selectedTracks.begin();
    while(it != selectedTracks.end())
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(accessor, *it);
        auto const groupIdentifier = groupAcsr.getAttr<Group::AttrType::identifier>();
        if(selectedGroups.count(groupIdentifier) > 0_z)
        {
            it = selectedTracks.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return std::make_tuple(std::move(selectedGroups), std::move(selectedTracks));
}

std::optional<juce::String> Document::Selection::getClosestItem(Accessor const& accessor, bool ignoreCollapsedTracks)
{
    auto const selectedGroups = getGroups(accessor);
    auto const selectedTracks = getTracks(accessor);
    auto const documentLayout = accessor.getAttr<AttrType::layout>();
    for(auto const& groupId : documentLayout)
    {
        if(Tools::hasGroupAcsr(accessor, groupId))
        {
            auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupId);
            if(!ignoreCollapsedTracks || groupAcsr.getAttr<Group::AttrType::expanded>())
            {
                auto const groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                for(auto const& trackId : groupLayout)
                {
                    if(selectedTracks.contains(trackId))
                    {
                        return trackId;
                    }
                }
            }
            if(selectedGroups.contains(groupId))
            {
                return groupId;
            }
        }
    }
    return {};
}

std::optional<juce::String> Document::Selection::getFarthestItem(Accessor const& accessor, bool ignoreCollapsedTracks)
{
    auto const selectedGroups = getGroups(accessor);
    auto const selectedTracks = getTracks(accessor);
    auto documentLayout = accessor.getAttr<AttrType::layout>();
    std::reverse(documentLayout.begin(), documentLayout.end());
    for(auto const& groupId : documentLayout)
    {
        if(Tools::hasGroupAcsr(accessor, groupId))
        {
            auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupId);
            if(!ignoreCollapsedTracks || groupAcsr.getAttr<Group::AttrType::expanded>())
            {
                auto groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                std::reverse(groupLayout.begin(), groupLayout.end());
                for(auto const& trackId : groupLayout)
                {
                    if(selectedTracks.contains(trackId))
                    {
                        return trackId;
                    }
                }
            }
            if(selectedGroups.contains(groupId))
            {
                return groupId;
            }
        }
    }
    return {};
}

std::optional<juce::String> Document::Selection::getNextItem(Accessor const& accessor, bool ignoreCollapsedTracks)
{
    auto const selectedGroups = getGroups(accessor);
    auto const selectedTracks = getTracks(accessor);
    auto documentLayout = accessor.getAttr<AttrType::layout>();
    if(documentLayout.empty())
    {
        return {};
    }
    juce::String identifier = documentLayout.front();
    std::reverse(documentLayout.begin(), documentLayout.end());
    for(auto const& groupId : documentLayout)
    {
        if(Tools::hasGroupAcsr(accessor, groupId))
        {
            auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupId);
            if(!ignoreCollapsedTracks || groupAcsr.getAttr<Group::AttrType::expanded>())
            {
                auto groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
                std::reverse(groupLayout.begin(), groupLayout.end());
                for(auto const& trackId : groupLayout)
                {
                    if(selectedTracks.contains(trackId))
                    {
                        return identifier;
                    }
                    identifier = trackId;
                }
            }
            if(selectedGroups.contains(groupId))
            {
                return identifier;
            }
            identifier = groupId;
        }
    }
    return identifier;
}

ANALYSE_FILE_END
