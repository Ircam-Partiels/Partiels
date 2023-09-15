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
    return copy_with_erased_if(accessor.getAttr<AttrType::tracks>(), [&](auto const& trackAcsr)
                               {
                                   auto const identifier = trackAcsr.get().template getAttr<Track::AttrType::identifier>();
                                   return std::none_of(layout.cbegin(), layout.cend(), [&](auto const& layoutId)
                                                       {
                                                           return layoutId == identifier;
                                                       });
                               });
}

std::vector<Group::ChannelVisibilityState> Group::Tools::getChannelVisibilityStates(Accessor const& accessor)
{
    std::vector<ChannelVisibilityState> channelslayout;
    auto const trackAcsrs = getTrackAcsrs(accessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        auto const& trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
        for(size_t i = 0; i < channelslayout.size(); ++i)
        {
            if(i < trackChannelsLayout.size() && channelslayout[i] != ChannelVisibilityState::both)
            {
                if(channelslayout[i] != static_cast<int>(trackChannelsLayout[i]))
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
    return channelslayout;
}

bool Group::Tools::isSelected(Accessor const& accessor)
{
    if(getChannelVisibilityStates(accessor).empty())
    {
        return accessor.getAttr<AttrType::focused>().any();
    }
    return !getSelectedChannels(accessor).empty();
}

std::set<size_t> Group::Tools::getSelectedChannels(Accessor const& accessor)
{
    auto const& states = accessor.getAttr<AttrType::focused>();
    auto const maxChannels = std::min(getChannelVisibilityStates(accessor).size(), states.size());
    std::set<size_t> channels;
    for(auto index = 0_z; index < maxChannels; ++index)
    {
        if(states[index])
        {
            channels.insert(index);
        }
    }
    return channels;
}

std::optional<size_t> Group::Tools::getChannel(Accessor const& accessor, juce::Rectangle<int> const& bounds, int y, bool ignoreSeparator)
{
    auto const verticalRanges = getChannelVerticalRanges(accessor, bounds);
    auto const it = std::find_if(verticalRanges.cbegin(), verticalRanges.cend(), [&](auto const& pair)
                                 {
                                     return ignoreSeparator ? y < pair.second.getEnd() : pair.second.contains(y);
                                 });
    return it != verticalRanges.cend() ? it->first : std::optional<size_t>{};
}

std::map<size_t, juce::Range<int>> Group::Tools::getChannelVerticalRanges(Accessor const& accessor, juce::Rectangle<int> bounds)
{
    auto const channelsLayout = getChannelVisibilityStates(accessor);
    auto const numVisibleChannels = static_cast<int>(std::count_if(channelsLayout.cbegin(), channelsLayout.cend(), [](auto const state)
                                                                   {
                                                                       return state != ChannelVisibilityState::hidden;
                                                                   }));
    if(numVisibleChannels == 0)
    {
        return {};
    }

    std::map<size_t, juce::Range<int>> verticalRanges;

    auto fullHeight = static_cast<float>(bounds.getHeight() - (numVisibleChannels - 1));
    auto const channelHeight = fullHeight / static_cast<float>(numVisibleChannels);
    auto remainder = 0.0f;

    auto channelCounter = 0;
    for(auto channel = 0_z; channel < channelsLayout.size(); ++channel)
    {
        if(channelsLayout[channel] != ChannelVisibilityState::hidden)
        {
            ++channelCounter;
            auto const currentHeight = std::min(channelHeight, fullHeight) + remainder;
            remainder = channelHeight - std::round(currentHeight);
            fullHeight -= std::round(currentHeight);
            auto region = bounds.removeFromTop(static_cast<int>(std::round(currentHeight)));
            if(channelCounter != numVisibleChannels)
            {
                region.removeFromBottom(1);
            }
            verticalRanges[channel] = region.getVerticalRange();
        }
    }
    return verticalRanges;
}

std::optional<std::reference_wrapper<Track::Accessor>> Group::Tools::getZoomTrackAcsr(Accessor const& accessor)
{
    auto const selectedTrack = getTrackAcsr(accessor, accessor.getAttr<AttrType::zoomid>());
    if(selectedTrack.has_value())
    {
        return selectedTrack;
    }
    for(auto const& trackIdentifier : accessor.getAttr<AttrType::layout>())
    {
        auto const trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>() && Track::Tools::getFrameType(trackAcsr.value()) != Track::FrameType::label)
        {
            return trackAcsr;
        }
    }
    return {};
}

bool Group::Tools::canZoomIn(Accessor const& accessor)
{
    auto const trackAcsr = getZoomTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        return Track::Tools::canZoomIn(trackAcsr.value().get());
    }
    return false;
}

bool Group::Tools::canZoomOut(Accessor const& accessor)
{
    auto const trackAcsr = getZoomTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        return Track::Tools::canZoomOut(trackAcsr.value().get());
    }
    return false;
}

void Group::Tools::zoomIn(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const trackAcsr = getZoomTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        Track::Tools::zoomIn(trackAcsr.value().get(), ratio, notification);
    }
}

void Group::Tools::zoomOut(Accessor& accessor, double ratio, NotificationType notification)
{
    auto const trackAcsr = getZoomTrackAcsr(accessor);
    if(trackAcsr.has_value())
    {
        Track::Tools::zoomOut(trackAcsr.value().get(), ratio, notification);
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
            case AttrType::zoomid:
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
