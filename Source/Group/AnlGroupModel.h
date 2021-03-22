#pragma once

#include "../Misc/AnlMisc.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Track/AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    enum class AttrType : size_t
    {
          name
        , height
        , layout
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::height, int, Model::Flag::basic>
    , Model::Attr<AttrType::layout, std::vector<juce::String>, Model::Flag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {""}
                                 , {144}
                                 , {}))
        {
        }
    };
}

ANALYSE_FILE_END
