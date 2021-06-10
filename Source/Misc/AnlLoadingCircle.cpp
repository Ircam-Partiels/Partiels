#include "AnlLoadingCircle.h"
#include "AnlIconManager.h"

ANALYSE_FILE_BEGIN

LoadingCircle::LoadingCircle(juce::Image const& inactiveImage)
: mImageActive(IconManager::getIcon(IconManager::IconType::loading))
, mImageInactive(inactiveImage)
{
    setInterceptsMouseClicks(true, true);
}

void LoadingCircle::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto const localBounds = getLocalBounds().toFloat();
    g.setColour(findColour(ColourIds::foregroundColourId));
    juce::RectanglePlacement const placement(juce::RectanglePlacement::Flags::stretchToFit);
    if(isTimerRunning())
    {
        g.addTransform(juce::AffineTransform::rotation(mRotation, localBounds.getCentreX(), localBounds.getCentreY()));
        g.drawImageTransformed(mImageActive, placement.getTransformToFit(mImageActive.getBounds().toFloat(), localBounds), true);
    }
    else if(mImageInactive.isValid())
    {
        g.drawImageTransformed(mImageInactive, placement.getTransformToFit(mImageInactive.getBounds().toFloat(), localBounds), true);
    }
}

void LoadingCircle::timerCallback()
{
    mRotation = std::fmod(mRotation + 0.3141592654f, 6.2831853072f);
    repaint();
}

void LoadingCircle::setActive(bool state)
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

void LoadingCircle::setInactiveImage(juce::Image const& inactiveImage)
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

ANALYSE_FILE_END
