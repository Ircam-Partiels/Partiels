#pragma once

#include "AnlZoomStateModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace State
    {
        class Slider
        : public juce::Component
        {
        public:
            using Attribute = Model::Attribute;
            using Signal = Model::Signal;
            
            enum Orientation : bool
            {
                vertical,
                horizontal
            };
            
            Slider(Accessor& accessor, Orientation orientation);
            ~Slider() override;
        
            // juce::Component
            void resized() override;
        private:
            
            Accessor& mAccessor;
            Accessor::Listener mListener;
            juce::Slider mSlider;
            JUCE_COMPILER_WARNING("change with scroll bar")
            juce::Slider mIncDec;
        };
    }
}

ANALYSE_FILE_END
