#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    // clang-format off
    enum class AttrType : size_t
    {
          playback
        , startPlayhead
        , runningPlayhead
        , looping
        , loopRange
        , stopAtLoopEnd
        , gain
        , markers
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::playback, bool, Model::Flag::notifying>
    , Model::Attr<AttrType::startPlayhead, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::runningPlayhead, double, Model::Flag::notifying>
    , Model::Attr<AttrType::looping, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::loopRange, Zoom::Range, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::stopAtLoopEnd, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::gain, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::markers, std::set<double>, Model::Flag::notifying>
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
                                 , {false}
                                 , {1.0}
                                 , {}
                                 ))
        {
        }
    };
    // clang-format on
} // namespace Transport

ANALYSE_FILE_END
