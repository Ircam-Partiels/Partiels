#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    enum class AttrType : size_t
    {
          playback
        , startPlayhead
        , runningPlayhead
        , looping
        , loopRange
        , gain
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::playback, bool, Model::Flag::notifying>
    , Model::Attr<AttrType::startPlayhead, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::runningPlayhead, double, Model::Flag::notifying>
    , Model::Attr<AttrType::looping, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::loopRange, Zoom::Range, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::gain, double, Model::Flag::notifying | Model::Flag::saveable>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {false}
                                 , {0.0}
                                 , {0.0}
                                 , {false}
                                 , {Zoom::Range{0.0, 0.0}}
                                 , {1.0}
                                 ))
        {
        }
    };
}

ANALYSE_FILE_END
