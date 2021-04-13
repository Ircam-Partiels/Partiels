#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

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

ANALYSE_FILE_END
