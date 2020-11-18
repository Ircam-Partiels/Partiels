#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum AttrType : size_t
    {
        key,
        name,
        parameters,
        results
    };
    
    using Result = Vamp::Plugin::Feature;
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::AttrFlag::basic>
    , Model::Attr<AttrType::results, std::vector<Result>, Model::AttrFlag::notifying | Model::AttrFlag::saveable>
    >;

    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
    };
}

namespace XmlParser
{
    template<>
    void toXml<Analyzer::Result>(juce::XmlElement& xml, juce::Identifier const& attributeName, Analyzer::Result const& value);
    
    template<>
    auto fromXml<Analyzer::Result>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Analyzer::Result const& defaultValue)
    -> Analyzer::Result;
}

ANALYSE_FILE_END
