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
    , Model::Acsr<AttrType::analyzers, Analyzer::Accessor, Model::AttrFlag::basic, Model::resizable>
    >;
    
    class Sanitizer
    {
    public:
        Sanitizer() = default;
        virtual ~Sanitizer() = default;
        
        virtual Zoom::range_type getGlobalRangeForFile(juce::File const& file) const = 0;
    };
    
    //! @todo Check if the zoom state is well initialized
    //! @todo Use a default gain to 1
    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        void setSanitizer(Sanitizer* santizer);
        
        template <enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            Model::Accessor<Accessor, Container>::setAttr<type, value_v>(value, notification);
        }
        
        template <>
        void setAttr<AttrType::file, juce::File>(juce::File const& value, NotificationType notification);
        
        template <enum_type type>
        bool insertAccessor(long index, NotificationType notification)
        {
             return Model::Accessor<Accessor, Container>::insertAccessor<type>(index, notification);
        }
        
        template <>
        bool insertAccessor<AttrType::analyzers>(long index, NotificationType notification)
        {
            static const Analyzer::Container ctnr{
                {""}
                , {""}
                , {0}
                , {{}}
                , {}
                , {juce::Colours::black}
                , {Analyzer::ColorMap::Heat}
                ,  {}
            };
            
            auto accessor = std::make_unique<Analyzer::Accessor>(ctnr);
            return Model::Accessor<Accessor, Container>::insertAccessor<AttrType::analyzers>(index, std::move(accessor), notification);
        }
        
    private:
        Sanitizer* mSanitizer = nullptr;
    };
}

ANALYSE_FILE_END
