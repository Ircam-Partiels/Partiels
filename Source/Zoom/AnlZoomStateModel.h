#pragma once

#include "../Tools/AnlModelAccessor.h"
#include "../Tools/AnlBroadcaster.h"
#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace State
    {
        using range_type = juce::Range<double>;
        using AttrFlag = Model::AttrFlag;
        
        enum AttrType : size_t
        {
            visibleRange,
            globalRange,
            minimumLength
        };
        
        enum class SignalType
        {
            moveAnchorBegin,
            moveAnchorEnd,
            moveAnchorPerform
        };
        
        using Container = Model::Container
        <Model::Attr<AttrType::visibleRange, range_type, AttrFlag::all>
        ,Model::Attr<AttrType::globalRange, range_type, AttrFlag::notifying>
        ,Model::Attr<AttrType::minimumLength, double, AttrFlag::notifying>
        >;
        
        JUCE_COMPILER_WARNING("Implement XML parser methods")
        class Accessor
        : public Model::Accessor<Accessor, Container>
        , public Broadcaster<Accessor, SignalType>
        {
        public:
            using Model::Accessor<Accessor, Container>::Accessor;
            using enum_type = Model::Accessor<Accessor, Container>::enum_type;
            
            template <enum_type attribute, typename value_v>
            void setValue(value_v const& value, NotificationType notification)
            {
                Model::Accessor<Accessor, Container>::setValue<attribute, value_v>(value, notification);
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
