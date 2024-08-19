#pragma once

#include "../Zoom/MiscZoom.h"

MISC_FILE_BEGIN

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
        , autoScroll
        , gain
        , markers
        , magnetize
        , selection
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::playback, bool, Model::Flag::notifying>
    , Model::Attr<AttrType::startPlayhead, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::runningPlayhead, double, Model::Flag::notifying>
    , Model::Attr<AttrType::looping, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::loopRange, Zoom::Range, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::stopAtLoopEnd, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::autoScroll, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::gain, double, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::markers, std::set<double>, Model::Flag::notifying>
    , Model::Attr<AttrType::magnetize, bool, Model::Flag::notifying | Model::Flag::saveable>
    , Model::Attr<AttrType::selection, Zoom::Range, Model::Flag::notifying>
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
                                 , {true}
                                 , {1.0}
                                 , {}
                                 , {false}
                                 , {}
                                 ))
        {
        }
    };
    // clang-format on
} // namespace Transport

MISC_FILE_END
