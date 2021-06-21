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
                           return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                       });
}

bool Document::Tools::isTrackInGroup(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    return std::any_of(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                       {
                           return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
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
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != groupAcsrs.cend());
    return it->get();
}

Group::Accessor const& Document::Tools::getGroupAcsrForTrack(Accessor const& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                           {
                               return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
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
                               return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                           });
    anlStrongAssert(it != groupAcsrs.end());
    return it->get();
}

Group::Accessor& Document::Tools::getGroupAcsrForTrack(Accessor& accessor, juce::String const& identifier)
{
    auto const groupAcsrs = accessor.getAcsrs<AcsrType::groups>();
    auto it = std::find_if(groupAcsrs.begin(), groupAcsrs.end(), [&](auto const& groupAcsr)
                           {
                               return Group::Tools::hasTrackAcsr(groupAcsr.get(), identifier);
                           });
    anlStrongAssert(it != groupAcsrs.end());
    return it->get();
}

size_t Document::Tools::getTrackPosition(Accessor const& accessor, juce::String const& identifier)
{
    auto const& groupAcsr = getGroupAcsrForTrack(accessor, identifier);
    auto const& layout = groupAcsr.getAttr<Group::AttrType::layout>();
    auto it = std::find(layout.cbegin(), layout.cend(), identifier);
    anlStrongAssert(it != layout.cend());
    return static_cast<size_t>(std::distance(layout.cbegin(), it));
}

size_t Document::Tools::getGroupPosition(Accessor const& accessor, juce::String const& identifier)
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

Document::LayoutNotifier::LayoutNotifier(juce::String const name, Accessor& accessor, std::function<void(void)> fn)
: mAccessor(accessor)
, onLayoutUpdated(fn)
, mName(name)
{
    mListener.onAccessorInserted = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                auto listener = std::make_unique<Track::Accessor::SmartListener>(mName + "::" + typeid(*this).name(), mAccessor.getAcsr<AcsrType::tracks>(index), [this](Track::Accessor const& trackAcsr, Track::AttrType trackAttribute)
                                                                                 {
                                                                                     juce::ignoreUnused(trackAcsr);
                                                                                     switch(trackAttribute)
                                                                                     {
                                                                                         case Track::AttrType::identifier:
                                                                                         {
                                                                                             if(onLayoutUpdated != nullptr)
                                                                                             {
                                                                                                 onLayoutUpdated();
                                                                                             }
                                                                                         }
                                                                                         break;
                                                                                         case Track::AttrType::name:
                                                                                         case Track::AttrType::results:
                                                                                         case Track::AttrType::key:
                                                                                         case Track::AttrType::description:
                                                                                         case Track::AttrType::state:
                                                                                         case Track::AttrType::height:
                                                                                         case Track::AttrType::colours:
                                                                                         case Track::AttrType::channelsLayout:
                                                                                         case Track::AttrType::zoomLink:
                                                                                         case Track::AttrType::zoomAcsr:
                                                                                         case Track::AttrType::graphics:
                                                                                         case Track::AttrType::warnings:
                                                                                         case Track::AttrType::processing:
                                                                                         case Track::AttrType::focused:
                                                                                             break;
                                                                                     }
                                                                                 });
                mTrackListeners.emplace(mTrackListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                auto listener = std::make_unique<Group::Accessor::SmartListener>(mName + "::" + typeid(*this).name(), mAccessor.getAcsr<AcsrType::groups>(index), [this](Group::Accessor const& groupAcsr, Group::AttrType groupAttribute)
                                                                                 {
                                                                                     juce::ignoreUnused(groupAcsr);
                                                                                     switch(groupAttribute)
                                                                                     {
                                                                                         case Group::AttrType::identifier:
                                                                                         {
                                                                                             if(onLayoutUpdated != nullptr)
                                                                                             {
                                                                                                 onLayoutUpdated();
                                                                                             }
                                                                                         }
                                                                                         break;
                                                                                         case Group::AttrType::name:
                                                                                         case Group::AttrType::height:
                                                                                         case Group::AttrType::colour:
                                                                                         case Group::AttrType::expanded:
                                                                                         case Group::AttrType::layout:
                                                                                         case Group::AttrType::tracks:
                                                                                         case Group::AttrType::focused:
                                                                                             break;
                                                                                     }
                                                                                 });
                mGroupListeners.emplace(mGroupListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::tracks:
            {
                mTrackListeners.erase(mTrackListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                mGroupListeners.erase(mGroupListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::reader:
            case AttrType::viewport:
            case AttrType::path:
                break;
            case AttrType::layout:
            {
                if(onLayoutUpdated != nullptr)
                {
                    onLayoutUpdated();
                }
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::LayoutNotifier::~LayoutNotifier()
{
    mAccessor.removeListener(mListener);
}

ANALYSE_FILE_END
