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
        // clang-format off
        enum Orientation : bool
        {
              vertical
            , horizontal
        };
        // clang-format on

        ScrollBar(Accessor& accessor, Orientation orientation, bool isInversed = false);
        ~ScrollBar() override;

        // juce::Component
        void resized() override;

    private:
        // juce::ScrollBar::Listener
        void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        juce::ScrollBar mScrollBar;
        bool const mIsInversed;
    };
} // namespace Zoom

ANALYSE_FILE_END
