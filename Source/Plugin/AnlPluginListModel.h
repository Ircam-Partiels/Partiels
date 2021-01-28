
#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    enum class AttrType : size_t
    {
          keys
        , sortColumn
        , sortIsFowards
    };
    
    enum ColumnType
    {
          Name = 1
        , Feature
        , Maker
        , Version
        , Category
        , Details
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::keys, std::set<Plugin::Key>, Model::Flag::basic>
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

ANALYSE_FILE_END
