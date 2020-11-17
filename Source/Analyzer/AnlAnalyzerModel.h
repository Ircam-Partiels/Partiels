#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum AttrType : size_t
    {
        key,
        name,
        sampleRate,
        numChannels,
        parameters
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::sampleRate, double, Model::AttrFlag::basic>
    , Model::Attr<AttrType::numChannels, unsigned long, Model::AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::AttrFlag::basic>
    >;

    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
    };
}

ANALYSE_FILE_END
