#include "MiscSpinningIcon.h"
#include "MiscIcon.h"

MISC_FILE_BEGIN

SpinningIcon::SpinningIcon(ScalableImage const& spinningImage, ScalableImage const& inactiveImage)
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
    auto const scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto const width = juce::roundToInt(localBounds.getWidth() * scale);
    auto const height = juce::roundToInt(localBounds.getHeight() * scale);
    juce::RectanglePlacement const placement(juce::RectanglePlacement::Flags::stretchToFit);
    if(isTimerRunning())
    {
        auto const image = mImageActive.getImage(width, height);
        if(image.isValid())
        {
            g.drawImageTransformed(image, placement.getTransformToFit(image.getBounds().toFloat(), localBounds).rotated(mRotation, localBounds.getCentreX(), localBounds.getCentreY()), true);
        }
    }
    else if(mImageInactive.isValid())
    {
        auto const image = mImageInactive.getImage(width, height);
        if(image.isValid())
        {
            g.drawImageTransformed(image, placement.getTransformToFit(image.getBounds().toFloat(), localBounds), true);
        }
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

void SpinningIcon::setSpinningImage(ScalableImage const& spinningImage)
{
    if(spinningImage != mImageActive)
    {
        mImageActive = spinningImage;
        if(!isTimerRunning())
        {
            repaint();
        }
    }
}

void SpinningIcon::setInactiveImage(ScalableImage const& inactiveImage)
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
