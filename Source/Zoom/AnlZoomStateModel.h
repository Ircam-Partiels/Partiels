#pragma once

#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    JUCE_COMPILER_WARNING("remove state")
    namespace State
    {
        using range_type = juce::Range<double>;
        JUCE_COMPILER_WARNING("remove this using that is annoying")
        using AttrFlag = Model::AttrFlag;
        
        enum AttrType : size_t
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
        < Model::Attr<AttrType::globalRange, range_type, AttrFlag::notifying>
        , Model::Attr<AttrType::minimumLength, double, AttrFlag::notifying>
        , Model::Attr<AttrType::visibleRange, range_type, AttrFlag::all>
        >;
        
        class Accessor
        : public Model::Accessor<Accessor, Container>
        , public Broadcaster<Accessor, SignalType>
        {
        public:
            using Model::Accessor<Accessor, Container>::Accessor;
            using enum_type = Model::Accessor<Accessor, Container>::enum_type;
            
            template <enum_type type, typename value_v>
            void setValue(value_v const& value, NotificationType notification)
            {
                Model::Accessor<Accessor, Container>::setValue<type, value_v>(value, notification);
            }
            
            template <>
            void setValue<AttrType::visibleRange, range_type>(range_type const& value, NotificationType notification);
            template <>
            void setValue<AttrType::globalRange, range_type>(range_type const& value, NotificationType notification);
            template <>
            void setValue<AttrType::minimumLength, double>(double const& value, NotificationType notification);
            
        private:
            static range_type sanitize(range_type const& visible, range_type const& global, double minLength);
        };
    }
}

ANALYSE_FILE_END
