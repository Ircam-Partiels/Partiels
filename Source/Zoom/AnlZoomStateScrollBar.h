#pragma once

#include "AnlZoomStateModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace State
    {
        class ScrollBar
        : public juce::Component
        , private juce::ScrollBar::Listener
        {
        public:
            using Attribute = Model::Attribute;
            using Signal = Model::Signal;
            
            enum Orientation : bool
            {
                vertical,
                horizontal
            };
            
            ScrollBar(Accessor& accessor, Orientation orientation);
            ~ScrollBar() override;
        
            // juce::Component
            void resized() override;
            
        private:
            
            // juce::ScrollBar::Listener
            void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
            
            Accessor& mAccessor;
            Accessor::Listener mListener;
            juce::ScrollBar mScrollBar;
            juce::Slider mIncDec;
        };
    }
}

ANALYSE_FILE_END
