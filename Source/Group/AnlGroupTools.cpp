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

Group::LayoutNotifier::LayoutNotifier(Accessor& accessor, std::function<void(void)> fn)
: mAccessor(accessor)
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
                                        return std::addressof(it->accessor.get()) == std::addressof(trackAcsr.get());
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
                                        return &trackListener.accessor.get() == &trackAcsr.get();
                                    }))
                    {
                        mTrackListeners.emplace(trackAcsr.get(), [this](Track::Accessor const& cTrackAcsr, Track::AttrType cTrackAttribute)
                                                {
                                                    juce::ignoreUnused(cTrackAcsr);
                                                    switch(cTrackAttribute)
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
                    }
                }
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
