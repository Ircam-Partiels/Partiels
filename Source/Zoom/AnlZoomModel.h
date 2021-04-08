#pragma once

#include "../Misc/AnlBroadcaster.h"
#include "../Misc/AnlModel.h"

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
    , Model::Attr<AttrType::anchor, std::tuple<double, bool>, Model::Flag::notifying>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        
        Accessor(Range const range = {0.0, 0.0}, double const length = 0.0)
        : Accessor(AttrContainer({range}, {length}, {range}, {{range.getStart(), false}}))
        {
        }
        
        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::visibleRange)
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
            }
        }
        
    private:
        static Range sanitize(Range const& visible, Range const& global, double minLength);
    };
}

ANALYSE_FILE_END
