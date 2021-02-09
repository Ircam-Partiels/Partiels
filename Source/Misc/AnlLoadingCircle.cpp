#include "AnlLoadingCircle.h"

ANALYSE_FILE_BEGIN

LoadingCircle::LoadingCircle()
: mImageProcessing(juce::ImageCache::getFromMemory(BinaryData::chargement_png, BinaryData::chargement_pngSize))
, mImageReady(juce::ImageCache::getFromMemory(BinaryData::checked_png, BinaryData::checked_pngSize))
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
        g.drawImageTransformed(mImageProcessing, placement.getTransformToFit(mImageProcessing.getBounds().toFloat(), localBounds), true);
    }
    else
    {
        g.drawImageTransformed(mImageReady, placement.getTransformToFit(mImageReady.getBounds().toFloat(), localBounds), true);
    }
}

void LoadingCircle::timerCallback()
{
    mRotation = std::fmod(mRotation + 0.3141592654f, 6.2831853072f);
    repaint();
}

void LoadingCircle::setActive(bool state)
{
    if(state)
    {
        startTimer(50);
    }
    else
    {
        stopTimer();
        repaint();
    }
}

ANALYSE_FILE_END
