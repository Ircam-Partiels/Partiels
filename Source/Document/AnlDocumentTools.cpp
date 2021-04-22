#include "AnlDocumentTools.h"
#include "../Group/AnlGroupTools.h"

ANALYSE_FILE_BEGIN

bool Document::Tools::hasTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                       {
                           return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                       });
}

bool Document::Tools::hasGroupAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    return std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                       {
                           return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier || Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                       });
}

Track::Accessor const& Document::Tools::getTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                           {
                               return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != trackAcsrs.cend());
    return it->get();
}

Group::Accessor const& Document::Tools::getGroupAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                           {
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier || Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                           });
    anlStrongAssert(it != groupAcsrs.cend());
    return it->get();
}

Track::Accessor& Document::Tools::getTrackAcsr(Accessor& accessor, juce::String const& identifier)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto it = std::find_if(trackAcsrs.begin(), trackAcsrs.end(), [&](auto const& trackAcsr)
                           {
                               return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != trackAcsrs.end());
    return it->get();
}

Group::Accessor& Document::Tools::getGroupAcsr(Accessor& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
                           {
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier || Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                           });
    anlStrongAssert(it != groupAcsrs.end());
    return it->get();
}

size_t Document::Tools::getTrackPosition(Accessor& accessor, juce::String const& identifier)
{
    auto const& groupAcsr = getGroupAcsr(accessor, identifier);
    auto const& layout = groupAcsr.getAttr<Group::AttrType::layout>();
    auto it = std::find(layout.cbegin(), layout.cend(), identifier);
    anlStrongAssert(it != layout.cend());
    return static_cast<size_t>(std::distance(layout.cbegin(), it));
}

size_t Document::Tools::getGroupPosition(Accessor& accessor, juce::String const& identifier)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    auto it = std::find(layout.cbegin(), layout.cend(), identifier);
    anlStrongAssert(it != layout.cend());
    return static_cast<size_t>(std::distance(layout.cbegin(), it));
}

std::optional<juce::String> Document::Tools::getFocusedTrack(Accessor const& accessor)
{
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto trackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                {
                                    return trackAcsr.get().template getAttr<Track::AttrType::focused>();
                                });
    if(trackIt == trackAcsrs.cend())
    {
        return std::optional<juce::String>();
    }
    return trackIt->get().getAttr<Track::AttrType::identifier>();
}

std::optional<juce::String> Document::Tools::getFocusedGroup(Accessor const& accessor)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto groupIt = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                {
                                    return groupAcsr.get().template getAttr<Group::AttrType::focused>();
                                });
    if(groupIt == groupAcsrs.cend())
    {
        return std::optional<juce::String>();
    }
    return groupIt->get().getAttr<Group::AttrType::identifier>();
}

std::optional<juce::String> Document::Tools::getFocusedItem(Accessor const& accessor)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto trackIt = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                {
                                    return trackAcsr.get().template getAttr<Track::AttrType::focused>();
                                });
    if(trackIt != trackAcsrs.cend())
    {
        return trackIt->get().getAttr<Track::AttrType::identifier>();
    }
    auto groupIt = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                {
                                    return groupAcsr.get().template getAttr<Group::AttrType::focused>();
                                });
    if(groupIt != groupAcsrs.cend())
    {
        return groupIt->get().getAttr<Group::AttrType::identifier>();
    }
    return std::optional<juce::String>();
}

ANALYSE_FILE_END
