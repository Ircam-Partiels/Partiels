#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportModel.h"
#include "../Track/AnlTrackModel.h"
#include "../Group/AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    enum class AttrType : size_t
    {
          file
        , layout
    };
    
    enum class AcsrType : size_t
    {
          timeZoom
        , transport
        , groups
        , tracks
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::timeZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::transport, Transport::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::groups, Group::Accessor, Model::Flag::basic, Model::resizable>
    , Model::Acsr<AcsrType::tracks, Track::Accessor, Model::Flag::basic, Model::resizable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer({juce::File{}} , {}))

        template <acsr_enum_type type>
        size_t getAcsrPosition(typename std::tuple_element<static_cast<size_t>(type), acsr_container_type>::type::accessor_type const& other) const
        {
            if constexpr(type == AcsrType::groups)
            {
                auto const identifier = other.template getAttr<Group::AttrType::identifier>();
                auto const groupAcsrs = getAcsrs<AcsrType::groups>();
                auto const it = std::find_if(groupAcsrs.cbegin(), groupAcsrs.cend(), [&](auto const& groupAcsr)
                                             {
                                                 return groupAcsr.get().template getAttr<Group::AttrType::identifier>() == identifier;
                                             });
                if(it == groupAcsrs.cend())
                {
                    return npos;
                }
                return static_cast<size_t>(std::distance(groupAcsrs.cbegin(), it));
            }
            else if constexpr(type == AcsrType::tracks)
            {
                auto const identifier = other.template getAttr<Track::AttrType::identifier>();
                auto const trackAcsrs = getAcsrs<AcsrType::tracks>();
                auto const it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                             {
                                                 return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == identifier;
                                             });
                if(it == trackAcsrs.cend())
                {
                    return npos;
                }
                return static_cast<size_t>(std::distance(trackAcsrs.cbegin(), it));
            }
        }
    };
}

ANALYSE_FILE_END
