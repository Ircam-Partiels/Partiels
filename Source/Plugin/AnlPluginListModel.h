
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
        
        Description() = default;
        explicit Description(juce::String const&);
        explicit operator juce::String() const;
        bool operator==(Description const&) const;
        bool operator!=(Description const&) const;
    };
    
    using description_map_type = std::map<juce::String, Description>;
    
    using AttrFlag = Model::AttrFlag;
    
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
    < Model::Attr<AttrType::descriptions, description_map_type, AttrFlag::notifying>
    , Model::Attr<AttrType::sortColumn, ColumnType, AttrFlag::all>
    , Model::Attr<AttrType::sortIsFowards, bool, AttrFlag::all>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        using enum_type = Model::Accessor<Accessor, Container>::enum_type;
    };
}

ANALYSE_FILE_END
