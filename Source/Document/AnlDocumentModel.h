#pragma once

#include "../Tools/AnlModel.h"
#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomModel.h"
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
        analyzers
    };
    
    enum class Signal
    {
        movePlayhead
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::AttrFlag::notifying | Model::AttrFlag::saveable>
    , Model::Attr<AttrType::gain, double, Model::AttrFlag::notifying | Model::AttrFlag::saveable>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, Model::AttrFlag::notifying>
    , Model::Acsr<AttrType::timeZoom, Zoom::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Acsr<AttrType::analyzers, Analyzer::Accessor, Model::AttrFlag::basic, Model::resizable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <enum_type type>
        auto getDefaultModel() const
        {
            return Model::Accessor<Accessor, Container>::getDefaultModel<type>();
        }
        
        template <>
        auto getDefaultModel<AttrType::analyzers>() const
        {
            return Analyzer::Container{{""}, {""}, {0}, {{}}, {juce::Colours::black}, {Analyzer::ColorMap::Heat}, {}, {}};
        }
    };
}

ANALYSE_FILE_END
