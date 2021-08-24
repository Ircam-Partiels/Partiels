#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    // clang-format off
    enum class AttrType : size_t
    {
          sortColumn
        , sortIsFowards
    };
    
    enum ColumnType
    {
          name = 1
        , feature
        , maker
        , version
        , category
        , details
    };
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::sortColumn, ColumnType, Model::Flag::basic>
    , Model::Attr<AttrType::sortIsFowards, bool, Model::Flag::basic>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
    };
} // namespace PluginList

ANALYSE_FILE_END
