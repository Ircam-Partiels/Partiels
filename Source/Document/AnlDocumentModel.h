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
        auto getDefaultContainer() const
        {
            return Model::Accessor<Accessor, Container>::getDefaultContainer<type>();
        }
        
        template <>
        auto getDefaultContainer<AttrType::analyzers>() const
        {
            static const Analyzer::Container ctnr {
                  {""}
                , {""}
                , {0}
                , {{}}
                , {}
                , {juce::Colours::black}
                , {Analyzer::ColorMap::Heat}
                ,  {}
            };
            return ctnr;
        }
        
    private:
        Sanitizer* mSanitizer = nullptr;
    };
}

ANALYSE_FILE_END
