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
                                 , {}))
        {
        }
    };
}

ANALYSE_FILE_END
