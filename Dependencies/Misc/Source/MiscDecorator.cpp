#include "MiscDecorator.h"

MISC_FILE_BEGIN

Decorator::Decorator(juce::Component& content, int borderThickness, float cornerSize)
: mContent(content)
, mBorderThickness(borderThickness)
, mCornerSize(cornerSize)
{
    setInterceptsMouseClicks(false, true);
    addAndMakeVisible(mContent);
    colourChanged();
}

void Decorator::setHighlighted(bool state)
{
    if(state != mIsHighlighted)
    {
        mIsHighlighted = state;
        repaint();
    }
}

void Decorator::resized()
{
    mContent.setBounds(getLocalBounds().reduced(mBorderThickness));
}

void Decorator::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Decorator::paintOverChildren(juce::Graphics& g)
{
    auto const offset = static_cast<int>(std::ceil(static_cast<float>(mBorderThickness) * 0.5f));
    auto const bounds = getLocalBounds().reduced(offset).toFloat();
    juce::Path path;
    path.addRectangle(bounds);
    path.setUsingNonZeroWinding(false);
    path.addRoundedRectangle(bounds, mCornerSize);

    g.setColour(findColour(ColourIds::backgroundColourId));
    g.fillPath(path);

    g.setColour(findColour(mIsHighlighted ? ColourIds::highlightedBorderColourId : ColourIds::normalBorderColourId));
    g.drawRoundedRectangle(bounds, mCornerSize, static_cast<float>(mBorderThickness));
}

MISC_FILE_END
