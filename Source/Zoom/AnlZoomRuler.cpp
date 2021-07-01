#include "AnlZoomRuler.h"

ANALYSE_FILE_BEGIN

Zoom::Ruler::Ruler(Accessor& accessor, Orientation orientation, std::function<juce::String(double)> valueToString, int maxStringWidth)
: mAccessor(accessor)
, mOrientation(orientation)
, mValueToString(valueToString)
, mMaximumStringWidth(std::max(maxStringWidth, 0))
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::globalRange:
            case AttrType::minimumLength:
                break;
            case AttrType::anchor:
            case AttrType::visibleRange:
            {
                repaint();
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this](Grid::Accessor const& acsr, Grid::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };

    mAccessor.getAcsr<AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Zoom::Ruler::~Ruler()
{
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::grid>().removeListener(mGridListener);
}

void Zoom::Ruler::mouseDown(juce::MouseEvent const& event)
{
    if(onMouseDown != nullptr && !onMouseDown())
    {
        return;
    }

    auto const visibleRange = mAccessor.getAttr<AttrType::visibleRange>();
    mInitialValueRange = visibleRange;

    auto getAnchorPoint = [&]()
    {
        if(mOrientation == Orientation::vertical)
        {
            auto const height = static_cast<double>(getHeight() - 1);
            return (height - event.y) / height * visibleRange.getLength() + visibleRange.getStart();
        }
        auto const width = static_cast<double>(getWidth() - 1);
        return static_cast<double>(event.x) / width * visibleRange.getLength() + visibleRange.getStart();
    };

    auto getNavigationMode = [&]()
    {
        if(event.mods.isShiftDown())
        {
            return NavigationMode::translate;
        }
        else if(event.mods.isCommandDown())
        {
            return NavigationMode::select;
        }
        return NavigationMode::zoom;
    };

    mNavigationMode = getNavigationMode();
    switch(mNavigationMode)
    {
        case NavigationMode::translate:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(true, getAnchorPoint()), NotificationType::synchronous);
            event.source.enableUnboundedMouseMovement(true, false);
            mPrevMousePos = event.position;
        }
        break;
        case NavigationMode::zoom:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(true, getAnchorPoint()), NotificationType::synchronous);
            event.source.enableUnboundedMouseMovement(true, false);
        }
        break;
        case NavigationMode::select:
            break;
    }
}

void Zoom::Ruler::mouseDrag(juce::MouseEvent const& event)
{
    if(event.mods.isRightButtonDown())
    {
        return;
    }

    auto const isVertical = mOrientation == Orientation::vertical;
    switch(mNavigationMode)
    {
        case NavigationMode::translate:
        {
            auto const visibleRange = mAccessor.getAttr<AttrType::visibleRange>();
            auto const delta = isVertical ? mPrevMousePos.y - event.position.y : event.position.x - mPrevMousePos.x;
            auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
            auto const translation = static_cast<double>(delta) / static_cast<double>(size) * visibleRange.getLength();
            mAccessor.setAttr<AttrType::visibleRange>(visibleRange + translation, NotificationType::synchronous);
        }
        break;
        case NavigationMode::select:
        {
            mSelectedValueRange = calculateSelectedValueRange(event);
            repaint();
        }
        break;
        case NavigationMode::zoom:
        {
            auto const zoomDistance = static_cast<double>(isVertical ? event.getDistanceFromDragStartX() : event.getDistanceFromDragStartY());
            auto const zoomFactor = zoomDistance < 0.0 ? std::pow(1.005, std::abs(zoomDistance)) : 1.0 / std::pow(1.005, std::abs(zoomDistance));
            auto const anchor = std::get<1>(mAccessor.getAttr<AttrType::anchor>());
            auto const rangeStart = anchor - (anchor - mInitialValueRange.getStart()) * zoomFactor;
            auto const rangeEnd = anchor + (mInitialValueRange.getEnd() - anchor) * zoomFactor;

            mAccessor.setAttr<AttrType::visibleRange>(Range{rangeStart, rangeEnd}, NotificationType::synchronous);
        }
        break;
    }

    mPrevMousePos = event.position;
}

void Zoom::Ruler::mouseUp(juce::MouseEvent const& event)
{
    if(event.mods.isRightButtonDown())
    {
        return;
    }

    switch(mNavigationMode)
    {
        case NavigationMode::translate:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(false, std::get<1>(mAccessor.getAttr<AttrType::anchor>())), NotificationType::synchronous);
        }
        break;
        case NavigationMode::select:
        {
            mSelectedValueRange = calculateSelectedValueRange(event);
            mAccessor.setAttr<AttrType::visibleRange>(mSelectedValueRange, NotificationType::synchronous);
            auto const anchorPos = (mSelectedValueRange.getStart() + mSelectedValueRange.getEnd()) / 2.0;
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(true, anchorPos), NotificationType::synchronous);
        }
        break;
        case NavigationMode::zoom:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(false, std::get<1>(mAccessor.getAttr<AttrType::anchor>())), NotificationType::synchronous);
        }
        break;
    }

    mInitialValueRange = {};
    mSelectedValueRange = {0.0, 0.0};
    mPrevMousePos = {0, 0};
}

void Zoom::Ruler::mouseDoubleClick(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    if(onDoubleClick)
    {
        onDoubleClick();
    }
}

juce::Range<double> Zoom::Ruler::calculateSelectedValueRange(juce::MouseEvent const& event)
{
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
    auto const mouseStart = isVertical ? size - event.getMouseDownY() : event.getMouseDownX();
    auto const mouseEnd = isVertical ? size - event.y : event.x;

    auto const startValue = static_cast<double>(mouseStart) / static_cast<double>(size) * mInitialValueRange.getLength() + mInitialValueRange.getStart();
    auto const endValue = static_cast<double>(mouseEnd) / static_cast<double>(size) * mInitialValueRange.getLength() + mInitialValueRange.getStart();
    return {std::min(startValue, endValue), std::max(startValue, endValue)};
}

void Zoom::Ruler::paint(juce::Graphics& g)
{
    g.fillAll(findColour(backgroundColourId));
    auto const& gridAcsr = mAccessor.getAcsr<AcsrType::grid>();
    auto const& visibleRange = mAccessor.getAttr<AttrType::visibleRange>();
    if(visibleRange.isEmpty())
    {
        return;
    }

    auto const bounds = getLocalBounds();
    g.setColour(findColour(gridColourId));
    if(mOrientation == Orientation::vertical)
    {
        Grid::paintVertical(g, gridAcsr, visibleRange, bounds, mValueToString, Grid::Justification::left);
    }
    else
    {

        Grid::paintHorizontal(g, gridAcsr, visibleRange, bounds, mValueToString, mMaximumStringWidth, Grid::Justification::bottom);
    }

    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    if(std::get<0>(mAccessor.getAttr<AttrType::anchor>()))
    {
        g.setColour(findColour(anchorColourId, true));
        auto const anchor = std::get<1>(mAccessor.getAttr<AttrType::anchor>());
        auto const anchorRatio = (anchor - visibleRange.getStart()) / visibleRange.getLength();
        if(mOrientation == Orientation::vertical)
        {
            auto const anchorPosition = static_cast<int>(std::round((1.0f - anchorRatio) * static_cast<float>(height)));
            g.drawHorizontalLine(anchorPosition, 0.0f, static_cast<float>(width));
        }
        else
        {
            auto const anchorPosition = static_cast<int>(std::round(anchorRatio * static_cast<float>(width)));
            g.drawVerticalLine(anchorPosition, 0.0f, static_cast<float>(height));
        }
    }
    else if(!mSelectedValueRange.isEmpty())
    {
        auto getSelectionRectangle = [&]() -> juce::Rectangle<float>
        {
            auto const start = static_cast<float>((mSelectedValueRange.getStart() - visibleRange.getStart()) / visibleRange.getLength());
            auto const end = static_cast<float>((mSelectedValueRange.getEnd() - visibleRange.getStart()) / visibleRange.getLength());
            if(mOrientation == Orientation::vertical)
            {
                return {0.0f, (1.0f - end) * static_cast<float>(height), static_cast<float>(width), (end - start) * static_cast<float>(height)};
            }
            return {start * static_cast<float>(width), 0.0f, (end - start) * static_cast<float>(width), static_cast<float>(height)};
        };

        auto const selectionRect = getSelectionRectangle();
        g.setColour(findColour(selectionColourId).withMultipliedAlpha(0.1f));
        g.fillRect(selectionRect);
        g.setColour(findColour(selectionColourId));
        g.drawRect(selectionRect);
    }
}

ANALYSE_FILE_END
