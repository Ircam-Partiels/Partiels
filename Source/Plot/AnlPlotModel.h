#pragma once

#include "../Misc/AnlModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Plot
{
    using ColourMap = tinycolormap::ColormapType;
    
    enum class AttrType : size_t
    {
          height
        , colourPlain
        , colourMap
        , propertyState
        , propertyVisible
    };
    
    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::colourPlain, juce::Colour, Model::Flag::basic>
    , Model::Attr<AttrType::colourMap, ColourMap, Model::Flag::basic>
    , Model::Attr<AttrType::propertyState, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::propertyVisible, bool, Model::Flag::notifying>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::valueZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::binZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {80}
                                 , {juce::Colours::aliceblue}
                                 , {ColourMap::Inferno}
                                 , {}
                                 , {false}))
        {
        }
        
        template <acsr_enum_type type>
        bool insertAccessor(size_t index, NotificationType const notification)
        {
            if constexpr(type == AcsrType::valueZoom)
            {
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{Zoom::lowest(), Zoom::max()}, Zoom::epsilon()), notification);
            }
            else if constexpr(type == AcsrType::binZoom)
            {
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{0.0, Zoom::max()}, 1.0), notification);
            }
            return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, notification);
        }
    };
}

ANALYSE_FILE_END
