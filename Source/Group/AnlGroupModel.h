#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    using TrackList = std::vector<std::reference_wrapper<Track::Accessor>>;
    
    enum class AttrType : size_t
    {
          identifier
        , name
        , height
        , colour
        , expanded
        , layout
        , tracks
        , focused
    };
    
    enum class AcsrType : size_t
    {
          zoom
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::colour, juce::Colour, Model::Flag::basic>
    , Model::Attr<AttrType::expanded, bool, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    , Model::Attr<AttrType::tracks, TrackList, Model::Flag::notifying>
    , Model::Attr<AttrType::focused, bool, Model::Flag::notifying>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::zoom, Zoom::Accessor, Model::Flag::basic, 1>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {}
                                 , {}
                                 , {144}
                                 , {juce::Colours::black}
                                 , {true}
                                 , {}
                                 , {}
                                 , {false}
                                 ))
        {
        }
        
        template <acsr_enum_type type>
        bool insertAcsr(size_t index, NotificationType const notification)
        {
            if constexpr(type == AcsrType::zoom)
            {
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAcsr<AcsrType::zoom>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{0.0, 1.0}), notification);
            }
        }
    };
}

ANALYSE_FILE_END
