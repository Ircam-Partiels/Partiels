#pragma once

#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    using Range = juce::Range<double>;
    
    enum class AttrType : size_t
    {
        globalRange,
        minimumLength,
        visibleRange
    };
    
    enum class SignalType
    {
        moveAnchorBegin,
        moveAnchorEnd,
        moveAnchorPerform
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::globalRange, Range, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::minimumLength, double, Model::AttrFlag::notifying>
    , Model::Attr<AttrType::visibleRange, Range, Model::AttrFlag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        
        template <enum_type type, typename value_v, typename fake = void>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == Zoom::AttrType::visibleRange)
            {
                auto sanitize = [](Range const& visible, Range const& global, double minLength)
                {
                    return global.constrainRange(visible.withEnd(std::max(visible.getStart() + minLength, visible.getEnd())));
                };
                Anl::Model::Accessor<Accessor, Container>::setAttr<AttrType::visibleRange, Zoom::Range>(sanitize(value, getAttr<AttrType::globalRange>(), getAttr<AttrType::minimumLength>()), notification);
            }
            else
            {
                Model::Accessor<Accessor, Container>::setAttr<type, value_v>(value, notification);
                setAttr<AttrType::visibleRange, Zoom::Range>(getAttr<AttrType::visibleRange>(), notification);
            }
        }
        
    private:
        static Range sanitize(Range const& visible, Range const& global, double minLength);
    };
}

ANALYSE_FILE_END
