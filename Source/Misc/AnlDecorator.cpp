#include "AnlDecorator.h"

ANALYSE_FILE_BEGIN

Decorator::Decorator(juce::Component& content, int borderThickness, float cornerSize)
: mContent(content)
, mBorderThickness(borderThickness)
, mCornerSize(cornerSize)
{
    addAndMakeVisible(mContent);
}

void Decorator::resized()
{
    juce::BorderSize<int> const borderSize(mBorderThickness);
    mContent.setBounds(borderSize.subtractedFrom(getLocalBounds()));
}

void Decorator::paint(juce::Graphics& g)
{
    g.setColour(findColour(ColourIds::backgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), mCornerSize);
}

void Decorator::paintOverChildren(juce::Graphics& g)
{
    g.setColour(findColour(ColourIds::borderColourId));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), mCornerSize, static_cast<int>(mBorderThickness));
}

ANALYSE_FILE_END
