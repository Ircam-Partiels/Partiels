#pragma once

#include "MiscZoomGrid.h"

MISC_FILE_BEGIN

namespace Zoom
{
    using Range = juce::Range<double>;

    static auto constexpr lowest()
    {
        return static_cast<double>(std::numeric_limits<float>::lowest());
    }

    static auto constexpr max()
    {
        return static_cast<double>(std::numeric_limits<float>::max());
    }

    static auto constexpr epsilon()
    {
        return static_cast<double>(std::numeric_limits<float>::epsilon());
    }

    // clang-format off
    enum class AttrType : size_t
    {
          globalRange
        , minimumLength
        , visibleRange
        , anchor
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::globalRange, Range, Model::Flag::basic>
    , Model::Attr<AttrType::minimumLength, double, Model::Flag::basic>
    , Model::Attr<AttrType::visibleRange, Range, Model::Flag::basic>
    , Model::Attr<AttrType::anchor, std::tuple<bool, double>, Model::Flag::notifying>
    >;
    
    enum class AcsrType : size_t
    {
          grid
    };
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::grid, Grid::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;

        Accessor(Range const range = {0.0, 0.0}, double const length = 0.0)
        : Accessor(AttrContainer({range}, {length}, {range}, {{range.getStart(), false}}))
        {
        }

        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::globalRange)
            {
                setGlobalRange(value, notification);
            }
            if constexpr(type == AttrType::minimumLength)
            {
                setMinimumLength(value, notification);
            }
            else if constexpr(type == AttrType::visibleRange)
            {
                setVisibleRange(value, notification);
            }
            if constexpr(type == AttrType::anchor)
            {
                setAnchor(value, notification);
            }
        }

    private:
        void setGlobalRange(Range const& globalRange, NotificationType notification);
        void setMinimumLength(double const& minimumLength, NotificationType notification);
        void setVisibleRange(Range const& visibleRange, NotificationType notification);
        void setAnchor(std::tuple<bool, double> const& anchor, NotificationType notification);
    };
} // namespace Zoom

MISC_FILE_END
