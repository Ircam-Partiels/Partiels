#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"
#include "../Zoom/AnlZoomModel.h"
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum AttrType : size_t
    {
        key,
        name,
        feature,
        parameters,
        zoom,
        colour,
        colourMap,
        results
    };
    
    using Result = Vamp::Plugin::Feature;
    using ColorMap = tinycolormap::ColormapType;
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::feature, size_t, Model::AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::AttrFlag::basic>
    , Model::Acsr<AttrType::zoom, Zoom::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Attr<AttrType::colour, juce::Colour, Model::AttrFlag::basic>
    , Model::Attr<AttrType::colourMap, ColorMap, Model::AttrFlag::basic>
    , Model::Attr<AttrType::results, std::vector<Result>, Model::AttrFlag::notifying>
    >;

    class Accessor
    : public Model::Accessor<Accessor, Container>
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
