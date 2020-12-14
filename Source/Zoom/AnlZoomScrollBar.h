#pragma once

#include "AnlZoomModel.h"

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
              vertical
            , horizontal
        };
        
        ScrollBar(Accessor& accessor, Orientation orientation, bool isInversed = false);
        ~ScrollBar() override;
        
        // juce::Component
        void resized() override;
        
    private:
        
        // juce::ScrollBar::Listener
        void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        juce::ScrollBar mScrollBar;
        bool const mIsInversed;
    };
}

ANALYSE_FILE_END
