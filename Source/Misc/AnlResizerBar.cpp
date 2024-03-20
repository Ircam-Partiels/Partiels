#include "AnlResizerBar.h"

ANALYSE_FILE_BEGIN

ResizerBar::ResizerBar(Orientation const orientation, bool direction, juce::Range<int> const range)
: mOrientation(orientation)
, mDirection(direction)
, mRange(range)
{
    enablementChanged();
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
    juce::ignoreUnused(event);
    auto const isVertical = mOrientation == Orientation::vertical;
    mSavedPosition = isVertical ? (mDirection ? getX() : getRight()) : (mDirection ? getY() : getBottom());
    event.source.enableUnboundedMouseMovement(true, true);
}

void ResizerBar::mouseDrag(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
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

    event.source.enableUnboundedMouseMovement(true, true);
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
        onMoved(mDirection ? newPosition : mOrientation == Orientation::horizontal ? getParentHeight() - newPosition
                                                                                   : getParentWidth() - newPosition);
    }
}

void ResizerBar::mouseUp(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
    juce::ignoreUnused(event);
    event.source.enableUnboundedMouseMovement(false);
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
