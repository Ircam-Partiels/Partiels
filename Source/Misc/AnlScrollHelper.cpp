#include "AnlScrollHelper.h"

ANALYSE_FILE_BEGIN

ScrollHelper::Orientation ScrollHelper::mouseWheelMove(juce::MouseWheelDetails const& wheel)
{
    startTimer(100);
    if(mCanUpdateOrientation && std::abs(wheel.deltaX + wheel.deltaY) > std::numeric_limits<float>::epsilon())
    {
        mCanUpdateOrientation = false;
        mOrientation = std::abs(wheel.deltaY) >= std::abs(wheel.deltaX) ? Orientation::vertical : Orientation::horizontal;
    }
    return mOrientation;
}

void ScrollHelper::timerCallback()
{
    mCanUpdateOrientation = true;
    stopTimer();
}

ANALYSE_FILE_END
