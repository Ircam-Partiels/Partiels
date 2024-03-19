#include "AnlResizerBar.h"

ANALYSE_FILE_BEGIN

ResizerBar::ResizerBar(Orientation const orientation, bool direction, juce::Range<int> const range)
: mOrientation(orientation)
, mDirection(direction)
, mRange(range)
{
    enablementChanged();
}

int ResizerBar::getSavedPosition() const
{
    return mSavedPosition;
}

int ResizerBar::getNewPosition() const
{
    return mNewPosition;
}

void ResizerBar::paint(juce::Graphics& g)
{
    g.setColour(findColour(isMouseOverOrDragging() && isEnabled() ? ColourIds::activeColourId : ColourIds::inactiveColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
}

void ResizerBar::mouseDown(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
    event.source.enableUnboundedMouseMovement(true, true);
    auto const isVertical = mOrientation == Orientation::vertical;
    mSavedPosition = isVertical ? (mDirection ? getX() : getRight()) : (mDirection ? getY() : getBottom());
    if(onMouseDown != nullptr)
    {
        onMouseDown(event);
    }
}

void ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
    event.source.enableUnboundedMouseMovement(true, true);
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const offset = isVertical ? event.getDistanceFromDragStartX() : event.getDistanceFromDragStartY();
    mNewPosition = mRange.isEmpty() ? mSavedPosition + offset : mRange.clipValue(mSavedPosition + offset);
    if(onMouseDrag != nullptr)
    {
        onMouseDrag(event);
    }
    else
    {
        if(mDirection)
        {
            setTopLeftPosition(isVertical ? mNewPosition : getX(),
                               !isVertical ? mNewPosition : getY());
        }
        else
        {
            setTopLeftPosition(isVertical ? mNewPosition - getWidth() : getX(),
                               !isVertical ? mNewPosition - getHeight() : getY());
        }
    }
}

void ResizerBar::mouseUp(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
    event.source.enableUnboundedMouseMovement(false);
    if(onMouseUp != nullptr)
    {
        onMouseUp(event);
    }
}

void ResizerBar::enablementChanged()
{
    if(isEnabled())
    {
        auto const isVertical = mOrientation == Orientation::vertical;
        setMouseCursor(isVertical ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

ANALYSE_FILE_END
