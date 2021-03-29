#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Plugin/AnlPluginModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Track
{
    using ColourMap = tinycolormap::ColormapType;
    using WindowState = juce::String;
    
    struct ColourSet
    {
        ColourMap map = ColourMap::Inferno;
        juce::Colour foreground = juce::Colours::aliceblue;
        juce::Colour background = juce::Colours::black;
        
        inline bool operator==(ColourSet const& rhd) const noexcept
        {
            return map == rhd.map &&
            foreground == rhd.foreground &&
            background == rhd.background;
        }
        
        inline bool operator!=(ColourSet const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };
    
    enum class AttrType : size_t
    {
          identifier
        , name
        , key
        , description
        , state
        
        , height
        , colours
        , propertyState
        
        , results
        , graphics
        , time
        , warnings
        , processing
    };
    
    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };

    enum class WarningType
    {
          none
        , plugin
        , state
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::description, Plugin::Description, Model::Flag::basic>
    , Model::Attr<AttrType::state, Plugin::State, Model::Flag::basic>
    
    , Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::colours, ColourSet, Model::Flag::basic>
    , Model::Attr<AttrType::propertyState, WindowState, Model::Flag::basic>
    
    , Model::Attr<AttrType::results, std::shared_ptr<const std::vector<Plugin::Result>>, Model::Flag::notifying>
    , Model::Attr<AttrType::graphics, std::vector<juce::Image>, Model::Flag::notifying>
    , Model::Attr<AttrType::time, double, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, WarningType, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, std::tuple<bool, float, bool, float>, Model::Flag::notifying>
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
        : Accessor(AttrContainer(  {""}
                                 , {""}
                                 , {}
                                 , {}
                                 , {}
                                 , {120}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 , {0.0}
                                 , {WarningType::none}
                                 , {{false, 0.0, false, 0.0}}))
        {
        }
    };
}

namespace XmlParser
{
    template<>
    void toXml<Track::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::ColourSet const& value);
    
    template<>
    auto fromXml<Track::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::ColourSet const& defaultValue)
    -> Track::ColourSet;
}

ANALYSE_FILE_END
