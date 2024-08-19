#include "MiscSpinningIcon.h"
#include "MiscIcon.h"

MISC_FILE_BEGIN

SpinningIcon::SpinningIcon(juce::Image const& spinningImage, juce::Image const& inactiveImage)
{
    setSpinningImage(spinningImage);
    setInactiveImage(inactiveImage);
    setInterceptsMouseClicks(true, true);
}

void SpinningIcon::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    g.setColour(findColour(ColourIds::foregroundColourId));
    auto const localBounds = getLocalBounds().toFloat();
    juce::RectanglePlacement const placement(juce::RectanglePlacement::Flags::stretchToFit);
    if(isTimerRunning())
    {
        g.drawImageTransformed(mImageActive, placement.getTransformToFit(mImageActive.getBounds().toFloat(), localBounds).rotated(mRotation, localBounds.getCentreX(), localBounds.getCentreY()), true);
    }
    else if(mImageInactive.isValid())
    {
        g.drawImageTransformed(mImageInactive, placement.getTransformToFit(mImageInactive.getBounds().toFloat(), localBounds), true);
    }
}

void SpinningIcon::timerCallback()
{
    mRotation = std::fmod(mRotation + 0.3141592654f, 6.2831853072f);
    repaint();
}

bool SpinningIcon::isActive() const
{
    return isTimerRunning();
}

void SpinningIcon::setActive(bool state)
{
    if(state && !isTimerRunning())
    {
        startTimer(50);
        repaint();
    }
    else if(!state && isTimerRunning())
    {
        stopTimer();
        repaint();
    }
}

void SpinningIcon::setSpinningImage(juce::Image const& spinningImage)
{
    if(spinningImage != mImageActive)
    {
        mImageActive = spinningImage;
        MiscWeakAssert(mImageActive.hasAlphaChannel());
        if(!isTimerRunning())
        {
            repaint();
        }
    }
}

void SpinningIcon::setInactiveImage(juce::Image const& inactiveImage)
{
    if(inactiveImage != mImageInactive)
    {
        mImageInactive = inactiveImage;
        if(!isTimerRunning())
        {
            repaint();
        }
    }
}

MISC_FILE_END
