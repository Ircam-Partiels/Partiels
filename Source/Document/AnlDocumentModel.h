#pragma once

#include "../Tools/AnlModel.h"
#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Layout/AnlLayoutStrechableContainerSection.h"
#include "../Tools/AnlBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    enum AttrType : size_t
    {
        file,
        isLooping,
        gain,
        isPlaybackStarted,
        playheadPosition,
        timeZoom,
        layout,
        layoutHorizontal,
        analyzers
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::AttrFlag::notifying | Model::AttrFlag::saveable>
    , Model::Attr<AttrType::gain, double, Model::AttrFlag::notifying | Model::AttrFlag::saveable>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, Model::AttrFlag::notifying>
    , Model::Acsr<AttrType::timeZoom, Zoom::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Acsr<AttrType::layout, Layout::StrechableContainer::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Attr<AttrType::layoutHorizontal, int, Model::AttrFlag::saveable>
    , Model::Acsr<AttrType::analyzers, Analyzer::Accessor, Model::AttrFlag::basic, Model::resizable>
    >;
    
    //! @todo Check if the zoom state is well initialized
    //! @todo Use a default gain to 1
    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <enum_type type>
        bool insertAccessor(long index, NotificationType notification)
        {
             return Model::Accessor<Accessor, Container>::insertAccessor<type>(index, notification);
        }
        
        template <>
        bool insertAccessor<AttrType::analyzers>(long index, NotificationType notification)
        {
            auto constexpr min = std::numeric_limits<double>::lowest()  / 100.0;
            auto constexpr max = std::numeric_limits<double>::max() / 100.0;
            auto constexpr epsilon = std::numeric_limits<double>::epsilon() * 100.0;
            static const Zoom::Container zoomCtnr {
                {{min, max}}
                , {epsilon}
                , {{min, max}}
            };
            
            static const Analyzer::Container ctnr {
                  {""}
                , {""}
                , {0}
                , {{}}
                , {512}
                , {zoomCtnr}
                , {juce::Colours::black}
                , {Analyzer::ColorMap::Heat}
                ,  {}
            };
            
            auto accessor = std::make_unique<Analyzer::Accessor>(ctnr);
            return Model::Accessor<Accessor, Container>::insertAccessor<AttrType::analyzers>(index, std::move(accessor), notification);
        }
    };
}

ANALYSE_FILE_END
