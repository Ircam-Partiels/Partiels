#pragma once

#include "../Misc/AnlModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

//! @todo All the plot should go back to the analyzer
namespace Plot
{
    using ColourMap = tinycolormap::ColormapType;
    
    struct ColourSet
    {
        ColourMap map = ColourMap::Inferno;
        juce::Colour line = juce::Colours::aliceblue;
        juce::Colour background = juce::Colours::black;
        
        inline bool operator==(ColourSet const& rhd) const noexcept
        {
            return map == rhd.map &&
            line == rhd.line &&
            background == rhd.background;
        }
        
        inline bool operator!=(ColourSet const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    using WindowState = juce::String;

    enum class AttrType : size_t
    {
          height
        , colours
        , propertyState
    };
    
    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::colours, ColourSet, Model::Flag::basic>
    , Model::Attr<AttrType::propertyState, WindowState, Model::Flag::basic>
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
                                 , {}
                                 , {}))
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

namespace XmlParser
{
    template<>
    void toXml<Plot::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Plot::ColourSet const& value);
    
    template<>
    auto fromXml<Plot::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Plot::ColourSet const& defaultValue)
    -> Plot::ColourSet;
}

ANALYSE_FILE_END
