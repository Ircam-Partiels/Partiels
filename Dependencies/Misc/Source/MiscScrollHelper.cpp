#include "MiscScrollHelper.h"

MISC_FILE_BEGIN

void ScrollHelper::mouseWheelMove(juce::MouseWheelDetails const& wheel, std::function<void(Orientation orientation)> updated)
{
    MiscWeakAssert(updated != nullptr);
    startTimer(20);
    auto constexpr epsilon = std::numeric_limits<float>::epsilon();
    auto const absDeltaX = std::abs(wheel.deltaX);
    auto const absDeltaY = std::abs(wheel.deltaY);
    auto const orientation = absDeltaY >= absDeltaX ? Orientation::vertical : Orientation::horizontal;
    if(!mOrientation.has_value())
    {
        mOrientation = orientation;
        if(updated != nullptr) [[likely]]
        {
            updated(orientation);
        }
    }
    else if(!wheel.isInertial && (absDeltaX + absDeltaY) > epsilon)
    {
        if(mOrientation.value() != orientation)
        {
            if(mOrientation == Orientation::vertical && absDeltaY <= epsilon)
            {
                mOrientation = Orientation::horizontal;
                if(updated != nullptr) [[likely]]
                {
                    updated(orientation);
                }
            }
            else if(mOrientation == Orientation::horizontal && absDeltaX <= epsilon)
            {
                mOrientation = Orientation::vertical;
                if(updated != nullptr) [[likely]]
                {
                    updated(orientation);
                }
            }
        }
    }
}

void ScrollHelper::timerCallback()
{
    mOrientation.reset();
    stopTimer();
}

MISC_FILE_END
