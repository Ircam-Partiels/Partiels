#pragma once

#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    using Range = juce::Range<double>;
    
    enum class AttrType : size_t
    {
          globalRange
        , minimumLength
        , visibleRange
    };
    
    enum class SignalType
    {
          moveAnchorBegin
        , moveAnchorEnd
        , moveAnchorPerform
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::globalRange, Range, Model::Flag::notifying>
    , Model::Attr<AttrType::minimumLength, double, Model::Flag::notifying>
    , Model::Attr<AttrType::visibleRange, Range, Model::Flag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        
        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == Zoom::AttrType::visibleRange)
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
        
        void sendSignal(SignalType signal, juce::var value, NotificationType const notification)
        {
            Broadcaster<Accessor, SignalType>::sendSignal(signal, value, notification);
        }
        
    private:
        static Range sanitize(Range const& visible, Range const& global, double minLength);
    };
}

ANALYSE_FILE_END
