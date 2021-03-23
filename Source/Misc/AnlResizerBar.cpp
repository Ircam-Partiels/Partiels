#include "AnlResizerBar.h"

ANALYSE_FILE_BEGIN

ResizerBar::ResizerBar(Orientation const orientation, bool direction, juce::Range<int> const range)
: mOrientation(orientation)
, mDirection(direction)
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
    mSavedPosition = isVertical ? (mDirection ? getX() : getRight()) : (mDirection ? getY() : getBottom());
}

void ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    auto getRange = [&]() -> juce::Range<int>
    {
        if(mDirection)
        {
            return mRange;
        }
        else if(mOrientation == Orientation::horizontal)
        {
            return {getParentHeight() - mRange.getEnd(), getParentHeight() - mRange.getStart()};
        }
        return {getParentWidth() - mRange.getEnd(), getParentWidth() - mRange.getStart()};
    };
    
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const offset = isVertical ? event.getDistanceFromDragStartX() : event.getDistanceFromDragStartY();
    auto const newPosition = mRange.isEmpty() ? mSavedPosition + offset : getRange().clipValue(mSavedPosition + offset);
    if(mDirection)
    {
        setTopLeftPosition(isVertical ? newPosition : getX(),
                           !isVertical ? newPosition : getY());
    }
    else
    {
        setTopLeftPosition(isVertical ? newPosition - getWidth() : getX(),
                           !isVertical ? newPosition - getHeight() : getY());
    }
    
    if(onMoved != nullptr)
    {
        onMoved(mDirection ? newPosition : mOrientation == Orientation::horizontal ? getParentHeight() - newPosition : getParentWidth() - newPosition);
    }
}

ANALYSE_FILE_END
