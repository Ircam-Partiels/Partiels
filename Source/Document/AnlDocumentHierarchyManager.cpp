#include "AnlDocumentHierarchyManager.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::HierarchyManager::HierarchyManager(Accessor& accessor)
: mAccessor(accessor)
{
}

std::vector<Document::HierarchyManager::TrackInfo> Document::HierarchyManager::getAvailableTracksFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier) const
{
    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier))
    {
        return {};
    }
    if(!Track::Tools::supportsInputTrack(Tools::getTrackAcsr(mAccessor, ownerIdentifier), inputIdentifier))
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
                if(isTrackValidFor(ownerIdentifier, inputIdentifier, identifier))
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

bool Document::HierarchyManager::isTrackValidFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier, juce::String const& inputTrack) const
{
    std::function<bool(Track::Accessor const&)> const hasValidInheritance = [&](Track::Accessor const& inputTrackAcsr)
    {
        auto const& trackIdentifier = inputTrackAcsr.getAttr<Track::AttrType::identifier>();
        if(trackIdentifier == ownerIdentifier)
        {
            return false;
        }
        auto const& rootInputs = inputTrackAcsr.getAttr<Track::AttrType::inputs>();
        return std::none_of(rootInputs.cbegin(), rootInputs.cend(), [&](auto const& rootInput)
                            {
                                auto const& prevInputIdentifier = rootInput.second;
                                return prevInputIdentifier.isNotEmpty() && Tools::hasTrackAcsr(mAccessor, prevInputIdentifier) && !hasValidInheritance(Tools::getTrackAcsr(mAccessor, prevInputIdentifier));
                            });
    };

    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier) || !Tools::hasTrackAcsr(mAccessor, inputTrack))
    {
        return false;
    }
    auto const& ownerTrackAcsr = Tools::getTrackAcsr(mAccessor, ownerIdentifier);
    if(!Track::Tools::supportsInputTrack(ownerTrackAcsr, inputIdentifier))
    {
        return false;
    }
    auto const& inputs = ownerTrackAcsr.getAttr<Track::AttrType::description>().inputs;
    auto const inputIt = std::find_if(inputs.cbegin(), inputs.cend(), [&](auto const input)
                                      {
                                          return input.identifier == inputIdentifier;
                                      });
    if(inputIt == inputs.cend())
    {
        return false;
    }
    auto const expectedFrameType = Track::Tools::getFrameType(*inputIt);
    auto const& inputTrackAcsr = Tools::getTrackAcsr(mAccessor, inputTrack);
    auto const inputFrameType = Track::Tools::getFrameType(inputTrackAcsr);
    if(expectedFrameType.has_value() && (!inputFrameType.has_value() || inputFrameType.value() != expectedFrameType.value()))
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
