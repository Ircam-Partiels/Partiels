#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

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

    struct GridInfo
    {
        double tickReference{0.0};
        double ticksSpacing{1.0};
        size_t largeTickInterval{0_z};

        inline bool operator==(GridInfo const& rhd) const noexcept
        {
            return std::abs(tickReference - rhd.tickReference) < std::numeric_limits<double>::epsilon() && std::abs(ticksSpacing - rhd.ticksSpacing) < std::numeric_limits<double>::epsilon() && largeTickInterval == rhd.largeTickInterval;
        }

        inline bool operator!=(GridInfo const& rhd) const noexcept
        {
            return !(*this == rhd);
        }
    };

    // clang-format off
    enum class AttrType : size_t
    {
          globalRange
        , minimumLength
        , visibleRange
        , gridInfo
        , anchor
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::globalRange, Range, Model::Flag::basic>
    , Model::Attr<AttrType::minimumLength, double, Model::Flag::basic>
    , Model::Attr<AttrType::visibleRange, Range, Model::Flag::basic>
    , Model::Attr<AttrType::gridInfo, GridInfo, Model::Flag::basic>
    , Model::Attr<AttrType::anchor, std::tuple<bool, double>, Model::Flag::notifying>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;

        Accessor(Range const range = {0.0, 0.0}, double const length = 0.0)
        : Accessor(AttrContainer({range}, {length}, {range}, {GridInfo{}}, {{range.getStart(), false}}))
        {
        }

        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::anchor)
            {
                auto const position = getAttr<AttrType::globalRange>().clipValue(std::get<1>(value));
                Anl::Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::anchor, value_v>(std::make_tuple(std::get<0>(value), position), notification);
            }
            else if constexpr(type == AttrType::visibleRange)
            {
                auto sanitize = [](Range const& visible, Range const& global, double minLength)
                {
                    return global.constrainRange(visible.withEnd(std::max(visible.getStart() + minLength, visible.getEnd())));
                };
                Anl::Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::visibleRange, Zoom::Range>(sanitize(value, getAttr<AttrType::globalRange>(), getAttr<AttrType::minimumLength>()), notification);
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer>::setAttr<type, value_v>(value, notification);
                setAttr<AttrType::visibleRange, Zoom::Range>(getAttr<AttrType::visibleRange>(), notification);
                setAttr<AttrType::anchor, std::tuple<bool, double>>(getAttr<AttrType::anchor>(), notification);
            }
        }

    private:
        static Range sanitize(Range const& visible, Range const& global, double minLength);
    };
} // namespace Zoom

namespace XmlParser
{
    template <>
    void toXml<Zoom::GridInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Zoom::GridInfo const& value);

    template <>
    auto fromXml<Zoom::GridInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Zoom::GridInfo const& defaultValue)
        -> Zoom::GridInfo;
} // namespace XmlParser
ANALYSE_FILE_END
