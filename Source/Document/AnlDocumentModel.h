#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportModel.h"
#include "../Track/AnlTrackModel.h"
#include "../Group/AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    enum class AttrType : size_t
    {
          file
    };
    
    enum class AcsrType : size_t
    {
          timeZoom
        , transport
        , groups
        , tracks
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::file, juce::File, Model::Flag::basic>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::timeZoom, Zoom::Accessor, Model::Flag::basic, 1>
    , Model::Acsr<AcsrType::transport, Transport::Accessor, Model::Flag::basic, 1>
    , Model::Acsr<AcsrType::groups, Group::Accessor, Model::Flag::basic, Model::resizable>
    , Model::Acsr<AcsrType::tracks, Track::Accessor, Model::Flag::basic, Model::resizable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer({juce::File{}}))
        {
        }
    };
}

ANALYSE_FILE_END
