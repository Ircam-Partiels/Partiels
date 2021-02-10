#include "AnlResizerBar.h"

ANALYSE_FILE_BEGIN

ResizerBar::ResizerBar(Orientation const orientation, juce::Range<int> const range)
: mOrientation(orientation)
, mRange(range)
{
    setRepaintsOnMouseActivity(true);
}

void ResizerBar::paint(juce::Graphics& g)
{
    auto const isVertical = mOrientation == Orientation::vertical;
    setMouseCursor(isVertical ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
    if(isMouseOverOrDragging())
    {
        g.setColour(findColour(ColourIds::activeColourId));
    }
    else
    {
        g.setColour(findColour(ColourIds::inactiveColourId));
    }
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
}

void ResizerBar::mouseDown(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    auto const isVertical = mOrientation == Orientation::vertical;
    mSavedPosition = isVertical ? getX() : getY();
}

void ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const offset = isVertical ? event.getDistanceFromDragStartX() : event.getDistanceFromDragStartY();
    auto const newPosition = mRange.isEmpty() ? mSavedPosition + offset : mRange.clipValue(mSavedPosition + offset);
    setTopLeftPosition(isVertical ? newPosition : getX(), !isVertical ? newPosition : getY());
    if(onMoved != nullptr)
    {
        onMoved(newPosition);
    }
}

ANALYSE_FILE_END
