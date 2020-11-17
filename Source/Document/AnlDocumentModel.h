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
        movePlayhead,
        audioInstanceChanged
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::AttrFlag::basic>
    , Model::Attr<AttrType::gain, double, Model::AttrFlag::basic>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, Model::AttrFlag::notifying>
    , Model::Model<AttrType::timeZoom, Zoom::Container, Zoom::Accessor, Model::AttrFlag::saveable, 1>
    , Model::Model<AttrType::analyzers, Analyzer::Container, Analyzer::Accessor, Model::AttrFlag::basic, 0>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
    };
}

ANALYSE_FILE_END
