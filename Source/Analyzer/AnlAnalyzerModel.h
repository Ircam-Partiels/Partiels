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
          key
        , name
        , feature
        , parameters
        , zoomMode
        , colour
        , colourMap
        , resultsType
        , results
        , warnings
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
          analyse
        , image
        , time
    };
    
    enum class ResultsType
    {
          undefined = 0 //! The results are undefined (might not be supported)
        , points        //! The results are time points
        , segments      //! The results are segments
        , matrix        //! The results is a matrix
    };
    
    enum class WarningType
    {
          feature
        , zoomMode
        , resultType
    };
    
    using ColorMap = tinycolormap::ColormapType;
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::feature, size_t, Model::Flag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::Flag::basic>
    , Model::Attr<AttrType::zoomMode, ZoomMode, Model::Flag::basic>
    , Model::Attr<AttrType::colour, juce::Colour, Model::Flag::basic>
    , Model::Attr<AttrType::colourMap, ColorMap, Model::Flag::basic>
    , Model::Attr<AttrType::resultsType, ResultsType, Model::Flag::notifying>
    , Model::Attr<AttrType::results, std::vector<Plugin::Result>, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, std::map<WarningType, juce::String>, Model::Flag::notifying>
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
                                 , {""}
                                 , {0}
                                 , {{}}
                                 , {ZoomMode::plugin}
                                 , {juce::Colours::black}
                                 , {ColorMap::Inferno}
                                 , {ResultsType::undefined}
                                 , {}
                                 , {}))
        {
        }
        
        template <acsr_enum_type type>
        bool insertAccessor(size_t index, NotificationType notification)
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
