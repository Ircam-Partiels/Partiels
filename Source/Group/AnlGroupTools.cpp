#include "AnlGroupTools.h"

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
