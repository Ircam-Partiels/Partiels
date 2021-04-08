#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

std::optional<std::reference_wrapper<Track::Accessor>> Document::Tools::getTrack(Accessor& accessor, juce::String identifier)
{
    auto const& trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
    {
        return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
    });
    return it != trackAcsrs.cend() ? *it : std::optional<std::reference_wrapper<Track::Accessor>>{};
}

bool Document::Tools::hasTrack(Accessor const& accessor, juce::String identifier)
{
    auto const& trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    return std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
    {
        return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
    });
}

ANALYSE_FILE_END
