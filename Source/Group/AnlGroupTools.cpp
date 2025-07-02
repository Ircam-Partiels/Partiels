#include "AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"

ANALYSE_FILE_BEGIN

bool Group::Tools::hasTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const& layoutId)
                    {
                        return layoutId == identifier;
                    }))
    {
        return false;
    }
    auto const trackAcsrs = accessor.getAttr<AttrType::tracks>();
    return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                       {
                           return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                       });
}

std::optional<std::reference_wrapper<Track::Accessor>> Group::Tools::getTrackAcsr(Accessor const& accessor, juce::String const& identifier)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    if(std::none_of(layout.cbegin(), layout.cend(), [&](auto const& layoutId)
                    {
                        return layoutId == identifier;
                    }))
    {
        return std::optional<std::reference_wrapper<Track::Accessor>>{};
    }

    auto const trackAcsrs = accessor.getAttr<AttrType::tracks>();
    auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                           {
                               return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                           });
    return it != trackAcsrs.cend() ? *it : std::optional<std::reference_wrapper<Track::Accessor>>{};
}

std::vector<std::reference_wrapper<Track::Accessor>> Group::Tools::getTrackAcsrs(Accessor const& accessor)
{
    auto const& layout = accessor.getAttr<AttrType::layout>();
    std::vector<std::reference_wrapper<Track::Accessor>> trackAcsrs;
    for(auto const& trackIdentifier : layout)
    {
        auto const trackAcsr = getTrackAcsr(accessor, trackIdentifier);
        if(trackAcsr.has_value())
        {
            trackAcsrs.push_back(trackAcsr.value());
        }
    }
    return trackAcsrs;
}

std::vector<Group::ChannelVisibilityState> Group::Tools::getChannelVisibilityStates(Accessor const& accessor)
{
    std::vector<ChannelVisibilityState> channelslayout;
    auto const trackAcsrs = getTrackAcsrs(accessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(trackAcsr.get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const& trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
            for(size_t i = 0; i < channelslayout.size(); ++i)
            {
                if(i < trackChannelsLayout.size() && channelslayout[i] != ChannelVisibilityState::both)
                {
                    if(static_cast<int>(channelslayout[i]) != static_cast<int>(trackChannelsLayout[i]))
                    {
                        channelslayout[i] = ChannelVisibilityState::both;
                    }
                }
            }
            for(size_t i = channelslayout.size(); i < trackChannelsLayout.size(); ++i)
            {
                channelslayout.push_back(trackChannelsLayout[i] ? ChannelVisibilityState::visible : ChannelVisibilityState::hidden);
            }
        }
    }
    return channelslayout;
}

bool Group::Tools::isSelected(Accessor const& accessor)
{
    auto const& states = accessor.getAttr<AttrType::focused>();
    auto const referenceTrack = Tools::getReferenceTrackAcsr(accessor);
    if(!referenceTrack.has_value())
    {
        return states.any();
    }
    auto const maxChannels = std::min(referenceTrack.value().get().getAttr<Track::AttrType::channelsLayout>().size(), states.size());
    if(maxChannels == 0_z)
    {
        return states.any();
    }
    for(auto index = 0_z; index < maxChannels; ++index)
    {
        if(states.test(index))
        {
            return true;
        }
    }
    return false;
}

std::set<size_t> Group::Tools::getSelectedChannels(Accessor const& accessor)
{
    auto const referenceTrack = Tools::getReferenceTrackAcsr(accessor);
    if(!referenceTrack.has_value())
    {
        return {};
    }
    auto const& states = accessor.getAttr<AttrType::focused>();
    auto const maxChannels = std::min(referenceTrack.value().get().getAttr<Track::AttrType::channelsLayout>().size(), states.size());
    std::set<size_t> channels;
    for(auto index = 0_z; index < maxChannels; ++index)
    {
        if(states.test(index))
        {
            channels.insert(index);
        }
    }
    return channels;
}

std::optional<size_t> Group::Tools::getChannel(Accessor const& accessor, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator)
{
    auto const referenceTrack = Tools::getReferenceTrackAcsr(accessor);
    return referenceTrack.has_value() ? Track::Tools::getChannel(referenceTrack.value().get(), bounds, y, ignoreSeparator) : std::optional<size_t>{};
}

std::optional<std::reference_wrapper<Track::Accessor>> Group::Tools::getReferenceTrackAcsr(Accessor const& accessor)
{
    auto const selectedTrack = getTrackAcsr(accessor, accessor.getAttr<AttrType::referenceid>());
    if(selectedTrack.has_value())
    {
        return selectedTrack;
    }
    for(auto const& trackIdentifier : accessor.getAttr<AttrType::layout>())
    {
        auto const trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            return trackAcsr;
        }
    }
    return {};
}

bool Group::Tools::canZoomIn(Accessor const& accessor)
{
    auto const trackAcsr = getReferenceTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        return Track::Tools::canZoomIn(trackAcsr.value().get());
    }
    return false;
}

bool Group::Tools::canZoomOut(Accessor const& accessor)
{
    auto const trackAcsr = getReferenceTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        return Track::Tools::canZoomOut(trackAcsr.value().get());
    }
    return false;
}

void Group::Tools::zoomIn(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const trackAcsr = getReferenceTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        Track::Tools::zoomIn(trackAcsr.value().get(), ratio, notification);
    }
}

void Group::Tools::zoomOut(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const trackAcsr = getReferenceTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        Track::Tools::zoomOut(trackAcsr.value().get(), ratio, notification);
    }
}

void Group::Tools::fillMenuForTrackVisibility(Accessor const& accessor, juce::PopupMenu& menu, std::function<void(void)> startFn, std::function<void(void)> endFn)
{
    auto const layout = accessor.getAttr<AttrType::layout>();
    for(auto layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
    {
        auto const trackId = layout.at(layoutIndex);
        auto const trackAcsr = Tools::getTrackAcsr(accessor, trackId);
        if(trackAcsr.has_value())
        {
            auto const& trackName = trackAcsr.value().get().getAttr<Track::AttrType::name>();
            auto const itemLabel = trackName.isEmpty() ? juce::translate("Track IDX").replace("IDX", juce::String(layoutIndex + 1)) : trackName;
            auto const trackIdentifier = trackAcsr.value().get().getAttr<Track::AttrType::identifier>();
            auto const trackVisible = trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>();
            menu.addItem(itemLabel, true, trackVisible, [=, &accessor]()
                         {
                             auto localTrackAcsr = Group::Tools::getTrackAcsr(accessor, trackIdentifier);
                             if(startFn != nullptr)
                             {
                                 startFn();
                             }
                             if(localTrackAcsr.has_value())
                             {
                                 localTrackAcsr.value().get().setAttr<Track::AttrType::showInGroup>(!trackVisible, NotificationType::synchronous);
                             }
                             if(endFn != nullptr)
                             {
                                 endFn();
                             }
                         });
        }
    }
}

void Group::Tools::fillMenuForChannelVisibility(Accessor const& accessor, juce::PopupMenu& menu, std::function<void(void)> startFn, std::function<void(void)> endFn)
{
    auto const channelslayout = getChannelVisibilityStates(accessor);
    auto const numChannels = channelslayout.size();
    if(numChannels > 2_z)
    {
        auto const oneHidden = std::any_of(channelslayout.cbegin(), channelslayout.cend(), [](auto const state)
                                           {
                                               return state != ChannelVisibilityState::visible;
                                           });
        menu.addItem(juce::translate("All Channels"), oneHidden, !oneHidden, [=, &accessor]()
                     {
                         if(startFn != nullptr)
                         {
                             startFn();
                         }
                         for(auto const& trackIdentifer : accessor.getAttr<AttrType::layout>())
                         {
                             auto trackAcsr = getTrackAcsr(accessor, trackIdentifer);
                             if(trackAcsr.has_value())
                             {
                                 auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                                 std::fill(copy.begin(), copy.end(), true);
                                 trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                             }
                         }
                         if(endFn != nullptr)
                         {
                             endFn();
                         }
                     });

        auto const firstHidden = channelslayout[0_z] != ChannelVisibilityState::visible || std::any_of(std::next(channelslayout.cbegin()), channelslayout.cend(), [](auto const state)
                                                                                                       {
                                                                                                           return state == ChannelVisibilityState::visible;
                                                                                                       });
        menu.addItem(juce::translate("Channel 1 Only"), firstHidden, !firstHidden, [=, &accessor]()
                     {
                         if(startFn != nullptr)
                         {
                             startFn();
                         }
                         for(auto const& trackIdentifer : accessor.getAttr<AttrType::layout>())
                         {
                             auto trackAcsr = getTrackAcsr(accessor, trackIdentifer);
                             if(trackAcsr.has_value())
                             {
                                 auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                                 if(!copy.empty())
                                 {
                                     copy[0_z] = true;
                                     std::fill(std::next(copy.begin()), copy.end(), false);
                                     trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                                 }
                             }
                         }
                         if(endFn != nullptr)
                         {
                             endFn();
                         }
                     });
    }

    auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const colour = juce::Colour::contrasting(lookAndFeel.findColour(juce::PopupMenu::ColourIds::backgroundColourId), lookAndFeel.findColour(juce::PopupMenu::ColourIds::textColourId));
    for(size_t channel = 0_z; channel < channelslayout.size(); ++channel)
    {
        juce::PopupMenu::Item item(juce::translate("Channel CHINDEX").replace("CHINDEX", juce::String(channel + 1)));
        item.isEnabled = !channelslayout.empty();
        item.isTicked = channelslayout[channel] != ChannelVisibilityState::hidden;
        if(channelslayout[channel] == ChannelVisibilityState::both)
        {
            item.colour = colour;
        }
        auto const newState = channelslayout[channel] != ChannelVisibilityState::visible ? true : false;
        item.action = [=, &accessor]()
        {
            if(startFn != nullptr)
            {
                startFn();
            }
            for(auto const& trackIdentifer : accessor.getAttr<AttrType::layout>())
            {
                auto trackAcsr = getTrackAcsr(accessor, trackIdentifer);
                if(trackAcsr.has_value())
                {
                    auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                    if(channel < copy.size())
                    {
                        copy[channel] = newState;
                        if(std::none_of(copy.cbegin(), copy.cend(), [](auto const& state)
                                        {
                                            return state;
                                        }))
                        {
                            if(channel + 1_z < copy.size())
                            {
                                copy[channel + 1_z] = true;
                            }
                            else if(channel > 0_z)
                            {
                                copy[channel - 1_z] = true;
                            }
                        }
                        trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                    }
                }
            }
            if(endFn != nullptr)
            {
                endFn();
            }
        };
        menu.addItem(std::move(item));
    }
}

Group::LayoutNotifier::LayoutNotifier(Accessor& accessor, std::function<void(void)> fn, std::set<Track::AttrType> attributes)
: mAccessor(accessor)
, mAttributes(std::move(attributes))
, onLayoutUpdated(fn)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::focused:
            case AttrType::expanded:
            case AttrType::referenceid:
                break;
            case AttrType::layout:
            {
                if(onLayoutUpdated != nullptr)
                {
                    onLayoutUpdated();
                }
            }
            break;
            case AttrType::tracks:
            {
                auto const trackAcsrs = acsr.getAttr<AttrType::tracks>();
                auto it = mTrackListeners.begin();
                while(it != mTrackListeners.end())
                {
                    if(std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                    {
                                        return *it == nullptr || std::addressof((*it)->accessor.get()) == std::addressof(trackAcsr.get());
                                    }))
                    {
                        it = mTrackListeners.erase(it);
                        if(onLayoutUpdated != nullptr)
                        {
                            onLayoutUpdated();
                        }
                    }
                    else
                    {
                        ++it;
                    }
                }

                for(auto& trackAcsr : trackAcsrs)
                {
                    if(std::none_of(mTrackListeners.cbegin(), mTrackListeners.cend(), [&](auto const& trackListener)
                                    {
                                        return trackListener != nullptr && std::addressof(trackListener->accessor.get()) == std::addressof(trackAcsr.get());
                                    }))
                    {
                        auto listener = std::make_unique<Track::Accessor::SmartListener>(typeid(*this).name(), trackAcsr.get(), [this](Track::Accessor const& cTrackAcsr, Track::AttrType cTrackAttribute)
                                                                                         {
                                                                                             juce::ignoreUnused(cTrackAcsr);
                                                                                             if(mAttributes.count(cTrackAttribute) > 0_z)
                                                                                             {
                                                                                                 if(onLayoutUpdated != nullptr)
                                                                                                 {
                                                                                                     onLayoutUpdated();
                                                                                                 }
                                                                                             }
                                                                                         });
                        mTrackListeners.insert(std::move(listener));
                    }
                }
                anlWeakAssert(mTrackListeners.size() == trackAcsrs.size());
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::LayoutNotifier::~LayoutNotifier()
{
    mAccessor.removeListener(mListener);
}

ANALYSE_FILE_END
