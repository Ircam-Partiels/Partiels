#include "MiscZoomRuler.h"
#include "MiscZoomTools.h"

MISC_FILE_BEGIN

Zoom::Ruler::Ruler(Accessor& accessor, Orientation orientation, std::function<juce::String(double)> valueToString, int maxStringWidth)
: mAccessor(accessor)
, mOrientation(orientation)
, mValueToString(valueToString)
, mMaximumStringWidth(std::max(maxStringWidth, 0))
{
    mListener.onAttrChanged = [&]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
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

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Grid::Accessor const& acsr, [[maybe_unused]] Grid::AttrType attribute)
    {
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

Zoom::Ruler::NavigationMode Zoom::Ruler::getNavigationMode(juce::ModifierKeys const& modifiers)
{
    if(modifiers.isShiftDown())
    {
        return NavigationMode::translate;
    }
    else if(modifiers.isCommandDown())
    {
        return NavigationMode::select;
    }
    else if(modifiers.isAltDown())
    {
        return NavigationMode::zoom;
    }
    return NavigationMode::navigate;
}

void Zoom::Ruler::mouseDown(juce::MouseEvent const& orignalEvent)
{
    auto const event = orignalEvent.getEventRelativeTo(this);
    mMouseOverwritten = !getLocalBounds().contains(event.getPosition()) || (onMouseDown != nullptr && !onMouseDown(event));
    if(mMouseOverwritten)
    {
        return;
    }

    auto const visibleRange = mAccessor.getAttr<AttrType::visibleRange>();
    mInitialValueRange = visibleRange;

    auto const getAnchorPoint = [&]()
    {
        if(mOrientation == Orientation::vertical)
        {
            return Tools::getScaledValueFromHeight(mAccessor, *this, event.y);
        }
        return Tools::getScaledValueFromWidth(mAccessor, *this, event.x);
    };

    mNavigationMode = getNavigationMode(event.mods);
    mAccessor.setAttr<AttrType::anchor>(std::make_tuple(true, getAnchorPoint()), NotificationType::synchronous);
    event.source.enableUnboundedMouseMovement(mNavigationMode != NavigationMode::select, false);
    mPrevMousePos = event.position;
    repaint();
}

void Zoom::Ruler::mouseDrag(juce::MouseEvent const& orignalEvent)
{
    auto const event = orignalEvent.getEventRelativeTo(this);
    if(mMouseOverwritten || event.mods.isRightButtonDown())
    {
        return;
    }

    auto const isVertical = mOrientation == Orientation::vertical;
    switch(mNavigationMode)
    {
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
        case NavigationMode::translate:
        {
            auto const delta = isVertical ? event.mouseDownPosition.y - event.position.y : event.position.x - event.mouseDownPosition.x;
            auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
            auto const translation = static_cast<double>(delta) / static_cast<double>(size) * mInitialValueRange.getLength();
            mAccessor.setAttr<AttrType::visibleRange>(mInitialValueRange - translation, NotificationType::synchronous);
            mPrevMousePos = event.position;
        }
        break;
        case NavigationMode::select:
        {
            auto const size = static_cast<double>(isVertical ? getHeight() - 1 : getWidth() - 1);
            auto const downPosition = std::clamp(isVertical ? size - static_cast<double>(event.mouseDownPosition.y) : static_cast<double>(event.mouseDownPosition.x), 0.0, size);
            auto const newPosition = std::clamp(isVertical ? size - static_cast<double>(event.position.y) : static_cast<double>(event.position.x), 0.0, size);
            auto const visibleRange = mAccessor.getAttr<AttrType::visibleRange>();
            auto const startValue = downPosition / size * visibleRange.getLength() + visibleRange.getStart();
            auto const endValue = newPosition / size * visibleRange.getLength() + visibleRange.getStart();
            mSelectedValueRange = juce::Range<double>::between(startValue, endValue);
        }
        break;
        case NavigationMode::navigate:
        {
            auto const zoomDistance = static_cast<double>(isVertical ? event.getDistanceFromDragStartX() : event.getDistanceFromDragStartY());
            auto const zoomFactor = zoomDistance < 0.0 ? std::pow(1.005, std::abs(zoomDistance)) : 1.0 / std::pow(1.005, std::abs(zoomDistance));
            auto const anchor = std::get<1>(mAccessor.getAttr<AttrType::anchor>());
            auto const rangeStart = anchor - (anchor - mInitialValueRange.getStart()) * zoomFactor;
            auto const rangeEnd = anchor + (mInitialValueRange.getEnd() - anchor) * zoomFactor;
            auto const delta = isVertical ? event.mouseDownPosition.y - event.position.y : event.position.x - event.mouseDownPosition.x;
            auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
            auto const translation = static_cast<double>(delta) / static_cast<double>(size) * mInitialValueRange.getLength();
            mAccessor.setAttr<AttrType::visibleRange>(Range{rangeStart - translation, rangeEnd - translation}, NotificationType::synchronous);
        }
        break;
    }

    repaint();
}

void Zoom::Ruler::mouseUp(juce::MouseEvent const& orignalEvent)
{
    auto const event = orignalEvent.getEventRelativeTo(this);
    if(mMouseOverwritten || event.mods.isRightButtonDown())
    {
        return;
    }

    switch(mNavigationMode)
    {
        case NavigationMode::zoom:
        case NavigationMode::translate:
        case NavigationMode::navigate:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(false, std::get<1_z>(mAccessor.getAttr<AttrType::anchor>())), NotificationType::synchronous);
        }
        break;
        case NavigationMode::select:
        {
            mAccessor.setAttr<AttrType::anchor>(std::make_tuple(false, std::get<1_z>(mAccessor.getAttr<AttrType::anchor>())), NotificationType::synchronous);
            if(event.mouseWasDraggedSinceMouseDown())
            {
                mAccessor.setAttr<AttrType::visibleRange>(mSelectedValueRange, NotificationType::synchronous);
            }
        }
        break;
    }

    mInitialValueRange = {};
    mSelectedValueRange = {0.0, 0.0};
    mPrevMousePos = {0, 0};
    repaint();
}

void Zoom::Ruler::mouseDoubleClick(juce::MouseEvent const& orignalEvent)
{
    if(onDoubleClick)
    {
        onDoubleClick(orignalEvent.getEventRelativeTo(this));
    }
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
    auto const gridColour = findColour(gridColourId);
    if(!gridColour.isTransparent())
    {
        g.setColour(gridColour);
        if(mOrientation == Orientation::vertical)
        {
            Grid::paintVertical(g, gridAcsr, visibleRange, bounds, mValueToString, Grid::Justification::left);
        }
        else
        {

            Grid::paintHorizontal(g, gridAcsr, visibleRange, bounds, mValueToString, mMaximumStringWidth, Grid::Justification::bottom);
        }
    }

    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    if(std::get<0>(mAccessor.getAttr<AttrType::anchor>()))
    {
        auto const anchorColour = findColour(anchorColourId);
        if(!anchorColour.isTransparent())
        {
            g.setColour(juce::Colours::black);
            auto const anchor = std::get<1>(mAccessor.getAttr<AttrType::anchor>());
            if(mOrientation == Orientation::vertical)
            {
                auto const anchorPosition = Zoom::Tools::getScaledYFromValue(mAccessor, *this, anchor);
                g.drawHorizontalLine(static_cast<int>(std::round(anchorPosition)), 0.0f, static_cast<float>(width));
            }
            else
            {
                auto const anchorPosition = Zoom::Tools::getScaledXFromValue(mAccessor, *this, anchor);
                g.drawVerticalLine(static_cast<int>(std::round(anchorPosition)), 0.0f, static_cast<float>(height));
            }
        }
    }
    if(!mSelectedValueRange.isEmpty())
    {
        auto const selectionColour = findColour(selectionColourId);
        if(!selectionColour.isTransparent())
        {
            auto const getSelectionRectangle = [&]() -> juce::Rectangle<float>
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
            g.setColour(selectionColour.withMultipliedAlpha(0.1f));
            g.fillRect(selectionRect);
            g.setColour(selectionColour);
            g.drawRect(selectionRect);
        }
    }
}

MISC_FILE_END
