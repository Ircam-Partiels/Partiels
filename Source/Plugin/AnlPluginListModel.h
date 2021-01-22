
#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    enum class AttrType : size_t
    {
          plugins
        , sortColumn
        , sortIsFowards
    };
    
    enum ColumnType
    {
          Name = 1
        , Specialization
        , Maker
        , Version
        , Category
        , Details
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::plugins, std::map<Plugin::Key, Plugin::Description>, Model::Flag::basic>
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
