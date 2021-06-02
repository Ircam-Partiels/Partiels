#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace Grid
    {
        // clang-format off
        enum class NumericBase
        {
              binary = 0
            , octal
            , decimal
            , duadecimal
            , hexadecimal
            , vigedecimal
        };
        
        enum class AttrType : size_t
        {
              tickReference
            , tickBase
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::tickReference, double, Model::Flag::basic>
        , Model::Attr<AttrType::tickBase, NumericBase, Model::Flag::basic>
        >;
        // clang-format on
        
        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;
            
            Accessor(double tickReference = 0.0, NumericBase tickBase = NumericBase::decimal)
            : Accessor(AttrContainer({tickReference}, {tickBase}))
            {
            }
        };
        
        void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, juce::Justification justification);
        
        
        class PropertyPanel
        : public FloatingWindowContainer
        {
        public:
            PropertyPanel(Accessor& accessor);
            ~PropertyPanel() override;
            
            // juce::Component
            void resized() override;
            
        private:
            Accessor& mAccessor;
            Accessor::Listener mListener;
            
            PropertyNumber mPropertyTickReference;
            PropertyList mPropertyTickBase;
            
            static auto constexpr sInnerWidth = 300;
        };
    }
} // namespace Zoom

ANALYSE_FILE_END
