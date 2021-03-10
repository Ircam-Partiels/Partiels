#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Plugin/AnlPluginModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Analyzer
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
          feature
        , zoomMode
        , resultType
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
    
    , Model::Attr<AttrType::results, std::vector<Plugin::Result>, Model::Flag::notifying>
    , Model::Attr<AttrType::time, double, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, std::map<WarningType, juce::String>, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, bool, Model::Flag::notifying>
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
                                 , {0.0}
                                 , {}
                                 , {false}))
        {
        }
        
        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::results)
            {
                acquireResultsWrittingAccess();
                Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(value, notification);
                releaseResultsWrittingAccess();
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<type, value_v>(value, notification);
            }
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
        
        void releaseResultsReadingAccess();
        bool acquireResultsReadingAccess();
        bool canContinueToReadResults() const;
        
    private:
        
        void releaseResultsWrittingAccess();
        void acquireResultsWrittingAccess();
        
        std::atomic<int> mNumReadingAccess {false};
        std::atomic<bool> mRequireWrittingAccess {false};
    };
}

namespace XmlParser
{
    template<>
    void toXml<Analyzer::ColourSet>(juce::XmlElement& xml, juce::Identifier const& attributeName, Analyzer::ColourSet const& value);
    
    template<>
    auto fromXml<Analyzer::ColourSet>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Analyzer::ColourSet const& defaultValue)
    -> Analyzer::ColourSet;
}

ANALYSE_FILE_END
