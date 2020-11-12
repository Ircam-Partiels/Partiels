
#pragma once

#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    struct Description
    {
        juce::String name {}; //!< The name of the plugin
        juce::String maker {}; //!< The maker of the plugin
        unsigned int api {}; //!< The API version used by the plugin
        std::set<juce::String> categories {}; //!< The categories of the plugin
        juce::String details {}; //!< Further information about the plugin
        
        bool operator==(Description const&) const;
        bool operator!=(Description const&) const;
    };
    
    enum AttrType : size_t
    {
        descriptions,
        sortColumn,
        sortIsFowards
    };
    
    enum ColumnType
    {
        Name = 1,
        Maker = 2,
        Api = 3,
        Category = 4,
        Details = 5
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::descriptions, std::map<juce::String, Description>, Model::AttrFlag::basic>
    , Model::Attr<AttrType::sortColumn, ColumnType, Model::AttrFlag::basic>
    , Model::Attr<AttrType::sortIsFowards, bool, Model::AttrFlag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        using enum_type = Model::Accessor<Accessor, Container>::enum_type;
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
