#pragma once

#include "../Tools/AnlModel.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"
#include "../Zoom/AnlZoomModel.h"
#include <vamp-hostsdk/PluginHostAdapter.h>
#include "../../tinycolormap/include/tinycolormap.hpp"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum AttrType : size_t
    {
        key,
        name,
        feature,
        parameters,
        colour,
        colourMap,
        results,
        zoom
    };
    
    using Result = Vamp::Plugin::Feature;
    using ColorMap = tinycolormap::ColormapType;
    
    using Container = Model::Container
    < Model::Attr<AttrType::key, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::AttrFlag::basic>
    , Model::Attr<AttrType::feature, size_t, Model::AttrFlag::basic>
    , Model::Attr<AttrType::parameters, std::map<juce::String, double>, Model::AttrFlag::basic>
    , Model::Attr<AttrType::colour, juce::Colour, Model::AttrFlag::basic>
    , Model::Attr<AttrType::colourMap, ColorMap, Model::AttrFlag::basic>
    , Model::Attr<AttrType::results, std::vector<Result>, Model::AttrFlag::notifying>
    , Model::Acsr<AttrType::zoom, Zoom::Accessor, Model::AttrFlag::saveable, 1>
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
