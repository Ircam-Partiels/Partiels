#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Plugin/AnlPluginModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum class AttrType : size_t
    {
          name
        , key
        , description
        , state
        
        , zoomMode
        , colour
        , colourMap
        
        , results
        , warnings
        , processing
        
        , display
    };
    
    enum class AcsrType : size_t
    {
          valueZoom
        , binZoom
    };
    
    enum class ZoomMode
    {
          plugin    //! The zoom is based on the plugin informations
        , results   //! The zoom is based on the results dimension
        , custom    //! The zoom is defined by the user
    };
    
    enum class SignalType
    {
          image
        , time
    };
    
    enum class WarningType
    {
          feature
        , zoomMode
        , resultType
    };
    
    using ColorMap = tinycolormap::ColormapType;
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::description, Plugin::Description, Model::Flag::basic>
    , Model::Attr<AttrType::state, Plugin::State, Model::Flag::basic>
    
    , Model::Attr<AttrType::zoomMode, ZoomMode, Model::Flag::basic>
    , Model::Attr<AttrType::colour, juce::Colour, Model::Flag::basic>
    , Model::Attr<AttrType::colourMap, ColorMap, Model::Flag::basic>
    
    , Model::Attr<AttrType::results, std::vector<Plugin::Result>, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, std::map<WarningType, juce::String>, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, bool, Model::Flag::notifying>
    
    , Model::Attr<AttrType::display, juce::String, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::valueZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    , Model::Acsr<AcsrType::binZoom, Zoom::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {""}
                                 , {}
                                 , {}
                                 , {}
                                 , {ZoomMode::plugin}
                                 , {juce::Colours::black}
                                 , {ColorMap::Inferno}
                                 , {}
                                 , {}
                                 , {false}
                                 , {""}))
        {
        }
        
        template <acsr_enum_type type>
        bool insertAccessor(size_t index, NotificationType const notification)
        {
            if constexpr(type == AcsrType::valueZoom)
            {
                auto constexpr min = std::numeric_limits<double>::lowest()  / 100.0;
                auto constexpr max = std::numeric_limits<double>::max() / 100.0;
                auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{min, max}, epsilon), notification);
            }
            else if constexpr(type == AcsrType::binZoom)
            {
                return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, std::make_unique<Zoom::Accessor>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, 1.0), notification);
            }
            return Model::Accessor<Accessor, AttrContainer, AcsrContainer>::insertAccessor<type>(index, notification);
        }
        
        std::shared_ptr<juce::Image const> getImage() const;
        void setImage(std::shared_ptr<juce::Image> image, NotificationType notification);
        
        double getTime() const;
        void setTime(double time, NotificationType notification);
    private:
            
        AtomicManager<juce::Image> mImageManager;
        std::atomic<double> mTime;
    };
}

ANALYSE_FILE_END
