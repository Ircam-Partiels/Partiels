#include "AnlDocumentHierarchyManager.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::HierarchyManager::HierarchyManager(Accessor& accessor)
: mAccessor(accessor)
{
}

std::vector<Document::HierarchyManager::TrackInfo> Document::HierarchyManager::getAvailableTracksFor(juce::String const& ownerIdentifier) const
{
    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier))
    {
        return {};
    }
    if(!Track::Tools::supportsInputTrack(Tools::getTrackAcsr(mAccessor, ownerIdentifier)))
    {
        return {};
    }
    std::vector<TrackInfo> tracks;
    auto const groupLayout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& groupIdentifier : groupLayout)
    {
        if(Tools::hasGroupAcsr(mAccessor, groupIdentifier))
        {
            auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, groupIdentifier);
            auto const trackAcsrs = Group::Tools::getTrackAcsrs(groupAcsr);
            for(auto const& trackAcsr : trackAcsrs)
            {
                auto const identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                if(isTrackValidFor(ownerIdentifier, identifier))
                {
                    auto const trackName = trackAcsr.get().getAttr<Track::AttrType::name>();
                    auto const groupName = groupAcsr.getAttr<Group::AttrType::name>();
                    tracks.push_back({identifier, groupName, trackName});
                }
            }
        }
    }
    return tracks;
}

bool Document::HierarchyManager::isTrackValidFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier) const
{
    std::function<bool(Track::Accessor const&)> const hasValidInheritance = [&](Track::Accessor const& inputTrackAcsr)
    {
        auto const& trackIdentifier = inputTrackAcsr.getAttr<Track::AttrType::identifier>();
        if(trackIdentifier == ownerIdentifier)
        {
            return false;
        }
        auto const& subInputIdentifier = inputTrackAcsr.getAttr<Track::AttrType::input>();
        if(subInputIdentifier.isNotEmpty() && Tools::hasTrackAcsr(mAccessor, subInputIdentifier))
        {
            return hasValidInheritance(Tools::getTrackAcsr(mAccessor, subInputIdentifier));
        }
        return true;
    };

    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier) || !Tools::hasTrackAcsr(mAccessor, inputIdentifier))
    {
        return false;
    }
    auto const& ownerTrackAcsr = Tools::getTrackAcsr(mAccessor, ownerIdentifier);
    auto const ownerFrameType = Track::Tools::getFrameType(ownerTrackAcsr.getAttr<Track::AttrType::description>().input);
    if(!ownerFrameType.has_value())
    {
        return false;
    }
    auto const& inputTrackAcsr = Tools::getTrackAcsr(mAccessor, inputIdentifier);
    auto const inputFrameType = Track::Tools::getFrameType(inputTrackAcsr);
    if(!inputFrameType.has_value())
    {
        return false;
    }
    if(inputFrameType.value() != ownerFrameType.value())
    {
        return false;
    }
    return hasValidInheritance(inputTrackAcsr);
}

bool Document::HierarchyManager::hasAccessor(juce::String const& identifier) const
{
    return Tools::hasTrackAcsr(mAccessor, identifier);
}

Track::Accessor const& Document::HierarchyManager::getAccessor(juce::String const& identifier) const
{
    return Tools::getTrackAcsr(mAccessor, identifier);
}

ANALYSE_FILE_END
