#pragma once

#include "AnlZoomStateModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class ScrollBar
    : public juce::Component
    , private juce::ScrollBar::Listener
    {
    public:
        
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

ANALYSE_FILE_END
