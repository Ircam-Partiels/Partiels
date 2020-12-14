#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"
#include "../Zoom/AnlZoomModel.h"
#include "../../tinycolormap/include/tinycolormap.hpp"
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum class ZoomMode
    {
          plugin
        , results
        , custom
    };
    
    enum class AttrType : size_t
    {
          key
        , name
        , feature
        , parameters
        , zoomMode
        , zoom
        , colour
        , colourMap
        , results
    };
    
    enum class SignalType
    {
          analysisBegin
        , analysisUpdated
        , analysisEnd
    };
    
    struct Result
    : public Vamp::Plugin::Feature
    {
        using Vamp::Plugin::Feature::Feature;
        
        Result(Feature const& feature)
        : Vamp::Plugin::Feature(feature)
        {
        }
        
        inline bool operator==(Result const& other) const
        {
            return hasTimestamp == other.hasTimestamp &&
            timestamp == other.timestamp &&
            hasDuration == other.hasDuration &&
            duration == other.duration &&
            values.size() == other.values.size() &&
            std::equal(values.cbegin(), values.cend(), other.values.cbegin()) &&
            label == other.label;
        }
    };
    
    using ColorMap = tinycolormap::ColormapType;
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::feature, size_t, Model::AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::AttrFlag::basic>
    , Model::Attr<AttrType::zoomMode, ZoomMode, Model::AttrFlag::basic>
    , Model::Acsr<AttrType::zoom, Zoom::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Attr<AttrType::colour, juce::Colour, Model::AttrFlag::basic>
    , Model::Attr<AttrType::colourMap, ColorMap, Model::AttrFlag::basic>
    , Model::Attr<AttrType::results, std::vector<Result>, Model::AttrFlag::notifying>
    >;

    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <enum_type type>
        bool insertAccessor(long index, NotificationType notification)
        {
            if constexpr(type == AttrType::zoom)
            {
                auto constexpr min = std::numeric_limits<double>::lowest()  / 100.0;
                auto constexpr max = std::numeric_limits<double>::max() / 100.0;
                auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
                static const Zoom::Container ctnr
                {
                      {{min, max}}
                    , {epsilon}
                    , {{min, max}}
                };
                
                auto accessor = std::make_unique<Zoom::Accessor>(ctnr);
                return Model::Accessor<Accessor, Container>::insertAccessor<AttrType::zoom>(index, std::move(accessor), notification);
            }
            else
            {
                return Model::Accessor<Accessor, Container>::insertAccessor<type>(index, notification);
            }
        }
    };
}

ANALYSE_FILE_END
