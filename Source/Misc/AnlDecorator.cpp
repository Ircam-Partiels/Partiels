#include "AnlDecorator.h"

ANALYSE_FILE_BEGIN

Decorator::Decorator(juce::Component& content, int borderThickness, float cornerSize)
: mContent(content)
, mBorderThickness(borderThickness)
, mCornerSize(cornerSize)
{
    addAndMakeVisible(mContent);
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
    auto const offset = static_cast<int>(std::ceil(static_cast<float>(mBorderThickness) * 1.5f));
    juce::BorderSize<int> const borderSize(offset);
    mContent.setBounds(borderSize.subtractedFrom(getLocalBounds()));
}

void Decorator::paint(juce::Graphics& g)
{
    g.setColour(findColour(ColourIds::backgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), mCornerSize);
}

void Decorator::paintOverChildren(juce::Graphics& g)
{
    auto const offset = static_cast<int>(std::ceil(static_cast<float>(mBorderThickness) * 0.5f));
    g.setColour(findColour(mIsHighlighted ? ColourIds::highlightedBorderColourId : ColourIds::normalBorderColourId));
    g.drawRoundedRectangle(getLocalBounds().reduced(offset).toFloat(), mCornerSize, static_cast<float>(mBorderThickness));
}

ANALYSE_FILE_END
