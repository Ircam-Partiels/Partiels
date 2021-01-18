
#pragma once

#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    struct Description
    {
        juce::String name {};                       //!< The name of the plugin
        std::map<size_t, juce::String> features;    //!< The features of the plugin
        juce::String maker {};                      //!< The maker of the plugin
        unsigned int api {0};                       //!< The API version used by the plugin
        std::set<juce::String> categories;          //!< The categories of the plugin
        juce::String details;                       //!< Further information about the plugin
        
        bool operator==(Description const&) const;
        bool operator!=(Description const&) const;
    };
    
    enum class AttrType : size_t
    {
          descriptions
        , sortColumn
        , sortIsFowards
    };
    
    enum ColumnType
    {
          Plugin = 1
        , Feature
        , Maker
        , Api
        , Category
        , Details
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::descriptions, std::map<juce::String, Description>, Model::Flag::basic>
    , Model::Attr<AttrType::sortColumn, ColumnType, Model::Flag::basic>
    , Model::Attr<AttrType::sortIsFowards, bool, Model::Flag::basic>
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
    void toXml<PluginList::Description>(juce::XmlElement& xml, juce::Identifier const& attributeName, PluginList::Description const& value);

    template<>
    auto fromXml<PluginList::Description>(juce::XmlElement const& xml, juce::Identifier const& attributeName, PluginList::Description const& defaultValue)
    -> PluginList::Description;
}

ANALYSE_FILE_END
