#pragma once

#include "../Misc/AnlMisc.h"
#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Misc/AnlBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    enum class AttrType : size_t
    {
          file
        , isLooping
        , gain
        , isPlaybackStarted
        , playheadPosition
        , layoutHorizontal
        , layout
    };
    
    enum class AcsrType : size_t
    {
          timeZoom
        , analyzers
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::Flag::basic>
    , Model::Attr<AttrType::isLooping, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::gain, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::isPlaybackStarted, bool, Model::Flag::notifying>
    , Model::Attr<AttrType::playheadPosition, double, Model::Flag::notifying>
    , Model::Attr<AttrType::layoutHorizontal, int, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::timeZoom, Zoom::Accessor, Model::Flag::saveable, 1>
    , Model::Acsr<AcsrType::analyzers, Analyzer::Accessor, Model::Flag::basic, Model::resizable>
    >;
    
    //! @todo Use a default gain to 1
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {juce::File{}}
                                 , {false}
                                 , {1.0}
                                 , {false}
                                 , {0.0}
                                 , {144}
                                 , {}))
        {
        }
    };
}

ANALYSE_FILE_END
