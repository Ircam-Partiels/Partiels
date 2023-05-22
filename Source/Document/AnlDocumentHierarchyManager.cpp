#include "AnlDocumentHierarchyManager.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::HierarchyManager::HierarchyManager(Accessor& accessor)
: mAccessor(accessor)
{
}

std::map<juce::String, juce::String> Document::HierarchyManager::getAvailableTracksFor(juce::String const& current, Track::FrameType const frameType) const
{
    std::map<juce::String, juce::String> tracks;
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
                if(isTrackValidFor(current, frameType, identifier))
                {
                    auto const trackName = trackAcsr.get().getAttr<Track::AttrType::name>();
                    auto const groupName = groupAcsr.getAttr<Group::AttrType::name>();
                    tracks[identifier] = groupName + " - " + trackName;
                }
            }
        }
    }
    return tracks;
}

bool Document::HierarchyManager::isTrackValidFor(juce::String const& current, Track::FrameType const frameType, juce::String const& identifier) const
{
    std::function<bool(Track::Accessor const&)> const hasValidInheritance = [&](Track::Accessor const& trackAcsr)
    {
        auto const& trackIdentifier = trackAcsr.getAttr<Track::AttrType::identifier>();
        if(trackIdentifier == current)
        {
            return false;
        }
        auto const& input = trackAcsr.getAttr<Track::AttrType::input>();
        if(input.isNotEmpty() && Tools::hasTrackAcsr(mAccessor, input))
        {
            return hasValidInheritance(Tools::getTrackAcsr(mAccessor, input));
        }
        return true;
    };

    if(!Tools::hasTrackAcsr(mAccessor, identifier))
    {
        return false;
    }
    auto const& trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
    if(Track::Tools::getFrameType(trackAcsr) != frameType)
    {
        return false;
    }
    return hasValidInheritance(trackAcsr);
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
