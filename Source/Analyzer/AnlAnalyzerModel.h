#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"

namespace Vamp
{
    class Plugin;
}

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    using AttrFlag = Model::AttrFlag;
    
    enum AttrType : size_t
    {
        key,
        name,
        sampleRate,
        numChannels,
        parameters
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, AttrFlag::basic>
    , Model::Attr<AttrType::sampleRate, double, AttrFlag::basic>
    , Model::Attr<AttrType::numChannels, unsigned long, AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, AttrFlag::basic>
    >;
    
    enum class Signal
    {
        pluginInstanceChanged
    };
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, Signal>
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        using enum_type = Model::Accessor<Accessor, Container>::enum_type;
    };
}

ANALYSE_FILE_END
