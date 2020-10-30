#include "AnlZoomStateRuler.h"

ANALYSE_FILE_BEGIN

Zoom::State::Ruler::Ruler(Accessor& accessor, Orientation orientation, size_t primaryTickInterval, double tickReferenceValue, double tickPowerInterval, double divisionFactor, double maxStringWidth)
: mAccessor(accessor)
, mOrientation(orientation)
{
    setMaximumStringWidth(maxStringWidth);
    setPrimaryTickInterval(primaryTickInterval);
    setTickReferenceValue(tickReferenceValue);
    setTickPowerInterval(tickPowerInterval, divisionFactor);
    
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mReceiver.onSignal = [&](Accessor& acsr, Signal signal, juce::var value)
    {
        juce::ignoreUnused(acsr);
        switch (signal)
        {
            case Signal::moveAnchorBegin:
                mZooming = true;
                break;
            case Signal::moveAnchorEnd:
                mZooming = false;
                break;
            case Signal::moveAnchorPerform:
                break;
        }
        mAnchor = mFromZoomRange(static_cast<double>(value));
        repaint();
    };
    
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    mAccessor.addReceiver(mReceiver);
}

Zoom::State::Ruler::~Ruler()
{
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
}

void Zoom::State::Ruler::setPrimaryTickInterval(size_t primaryTickInterval)
{
    mPrimaryTickInterval = std::max(primaryTickInterval, static_cast<size_t>(0)) + 1;
    repaint();
}

size_t Zoom::State::Ruler::getPrimaryTickInterval() const
{
    return mPrimaryTickInterval - 1;
}

void Zoom::State::Ruler::setTickReferenceValue(double tickReferenceValue)
{
    mTickReferenceValue = tickReferenceValue;
    repaint();
}

double Zoom::State::Ruler::getTickReferenceValue() const
{
    return mTickReferenceValue;
}

void Zoom::State::Ruler::setTickPowerInterval(double tickPowerInterval, double divisionFactor)
{
    anlStrongAssert(tickPowerInterval >= 1.0);
    anlStrongAssert(divisionFactor >= 1.0);
    mTickPowerInterval = std::max(tickPowerInterval, 1.0);
    mTickPowerDivisionFactor = std::max(divisionFactor, 1.0);
    repaint();
}

double Zoom::State::Ruler::getTickPowerInterval() const
{
    return mTickPowerInterval;
}

double Zoom::State::Ruler::getTickPowerDivisionFactor() const
{
    return mTickPowerDivisionFactor;
}

void Zoom::State::Ruler::setMaximumStringWidth(double maxStringWidth)
{
    anlStrongAssert(maxStringWidth > 5.0);
    mMaximumStringWidth = std::max(maxStringWidth, 5.0);
    
    repaint();
}

void Zoom::State::Ruler::setValueAsStringMethod(std::function<juce::String(double)> valueAsStringMethod)
{
    mGetValueAsString = valueAsStringMethod;
    repaint();
}

void Zoom::State::Ruler::setRangeConversionMethods(std::function<double(double)> fromZoomToRange, std::function<double(double)> toZoomFromRange)
{
    mFromZoomRange = fromZoomToRange;
    mToZoomRange = toZoomFromRange;
    
    repaint();
}

void Zoom::State::Ruler::setOrientation(Orientation orientation)
{
    if(mOrientation != orientation)
    {
        mOrientation = orientation;
        resized();
    }
}

Zoom::State::Ruler::Orientation Zoom::State::Ruler::getOrientation()
{
    return mOrientation;
}

juce::String Zoom::State::Ruler::valueAsStringKiloShorthand(double value)
{
    return std::abs(value / 1000.0) < 1.0 ? juce::String(static_cast<int>(value)) : juce::String(static_cast<int>(value    / 1000.0)) + "K";
}

double Zoom::State::Ruler::fromZoomRangePassThrough(double input)
{
    return input;
}

double Zoom::State::Ruler::toZoomRangePassThrough(double input)
{
    return input;
}

void Zoom::State::Ruler::mouseDown(juce::MouseEvent const& event)
{
    if(event.mods.isRightButtonDown())
    {
        if(onRightClick != nullptr)
        {
            onRightClick();
        }
        
        return;
    }
    
    auto const visibleRange = mAccessor.getModel().visibleRange;
    mInitialValueRange = {mFromZoomRange(visibleRange.getStart()), mFromZoomRange(visibleRange.getEnd())};
       
    if(mInitialValueRange.isEmpty())
    {
        return;
    }
    
    auto const useableRangeAnchorPoint = mOrientation == Orientation::vertical ?
    static_cast<double>(getHeight() - 1 - event.y) / static_cast<double>(getHeight() - 1) * mInitialValueRange.getLength() + mInitialValueRange.getStart() :
    static_cast<double>(event.x) / static_cast<double>(getWidth() - 1) * mInitialValueRange.getLength() + mInitialValueRange.getStart();
    
    if(event.mods.isShiftDown())
    {
        mNavigationMode = NavigationMode::translate;
        mAccessor.sendSignal(Signal::moveAnchorBegin, {mToZoomRange(useableRangeAnchorPoint)}, juce::NotificationType::sendNotificationSync);
        event.source.enableUnboundedMouseMovement(true, false);
    }
    else if(event.mods.isCommandDown())
    {
        mNavigationMode = NavigationMode::select;
    }
    else
    {
        mNavigationMode = NavigationMode::zoom;
        mAccessor.sendSignal(Signal::moveAnchorBegin, {mToZoomRange(useableRangeAnchorPoint)}, juce::NotificationType::sendNotificationSync);
        event.source.enableUnboundedMouseMovement(true, false);
        
        if(onMouseDown != nullptr)
        {
            onMouseDown(event.x);
        }
    }
    
    mPrevMousePos = event.position;
}

void Zoom::State::Ruler::mouseDrag(juce::MouseEvent const& event)
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
            auto copy = mAccessor.getModel();
            auto const delta = isVertical ? mPrevMousePos.y - event.position.y : event.position.x - mPrevMousePos.x;
            auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
            auto const translation = static_cast<double>(delta) / static_cast<double>(size) * copy.visibleRange.getLength();
            copy.visibleRange += translation;
            mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
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
            auto const zoomIncrement = std::pow(1.005, std::abs(zoomDistance));
            auto const zoomFactor = zoomIncrement < 0.0 ? zoomIncrement : 1.0 / zoomIncrement;
            auto const rangeStart = mAnchor - (mAnchor - mInitialValueRange.getStart()) * zoomFactor;
            auto const rangeEnd = mAnchor + (mInitialValueRange.getEnd() - mAnchor) * zoomFactor;

            auto copy = mAccessor.getModel();
            copy.visibleRange = {mToZoomRange(rangeStart), mToZoomRange(rangeEnd)};
            mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
            mAccessor.sendSignal(Signal::moveAnchorPerform, {mToZoomRange(mAnchor)}, juce::NotificationType::sendNotificationSync);
        }
            break;
    }
    
    mPrevMousePos = event.position;
}

void Zoom::State::Ruler::mouseUp(juce::MouseEvent const& event)
{
    if(event.mods.isRightButtonDown())
    {
        return;
    }
    
    switch(mNavigationMode)
    {
        case NavigationMode::translate:
        {
            mAccessor.sendSignal(Signal::moveAnchorEnd, {mToZoomRange(mAnchor)}, juce::NotificationType::sendNotificationSync);
        }
            break;
        case NavigationMode::select:
        {
            mSelectedValueRange = calculateSelectedValueRange(event);
            auto copy = mAccessor.getModel();
            copy.visibleRange = mSelectedValueRange;
            mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
            auto const anchorPos = (mSelectedValueRange.getStart() + mSelectedValueRange.getEnd()) / 2.0;
            mAccessor.sendSignal(Signal::moveAnchorPerform, {mToZoomRange(anchorPos)}, juce::NotificationType::sendNotificationSync);
        }
            break;
        case NavigationMode::zoom:
        {
            mAccessor.sendSignal(Signal::moveAnchorEnd, mToZoomRange(mAnchor), juce::NotificationType::sendNotificationSync);
        }
            break;
    }
    
    mInitialValueRange = {};
    mSelectedValueRange = {0.0, 0.0};
    mPrevMousePos = {0, 0};
}

void Zoom::State::Ruler::mouseDoubleClick(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    if(onDoubleClick)
    {
        onDoubleClick();
    }
    else
    {
        auto copy = mAccessor.getModel();
        copy.visibleRange = copy.globalRange;
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    }
}

juce::Range<double> Zoom::State::Ruler::calculateSelectedValueRange(juce::MouseEvent const& event)
{
    auto const isVertical = mOrientation == Orientation::vertical;
    auto const size = isVertical ? getHeight() - 1 : getWidth() - 1;
    auto const mouseStart = isVertical ? size - event.getMouseDownY() : event.getMouseDownX();
    auto const mouseEnd = isVertical ? size - event.y : event.x;
    
    auto const startValue = static_cast<double>(mouseStart) / static_cast<double>(size) * mInitialValueRange.getLength() + mInitialValueRange.getStart();
    auto const endValue = static_cast<double>(mouseEnd) / static_cast<double>(size) * mInitialValueRange.getLength() + mInitialValueRange.getStart();
    return { mToZoomRange(std::min(startValue, endValue)), mToZoomRange(std::max(startValue, endValue)) };
}

void Zoom::State::Ruler::paint(juce::Graphics &g)
{
    g.fillAll(findColour(backgroundColourId));
    auto const visibleRange = mAccessor.getModel().visibleRange;
    
    if(visibleRange.isEmpty())
    {
        return;
    }
    
    juce::Range<double> const useableRange { mFromZoomRange(visibleRange.getStart()), mFromZoomRange(visibleRange.getEnd()) };
    auto const useableRangeLength = useableRange.getLength();
    
    if(useableRange.isEmpty())
    {
        return;
    }
    
    auto const isVerticallyOriented = mOrientation == Orientation::vertical;
    auto const size = isVerticallyOriented ? getHeight() - 1 : getWidth() - 1;
    auto const textPadding = isVerticallyOriented ? 0 : 4;
    
    // The minimum interval depends on the font height
    auto const font = g.getCurrentFont();
    auto const minTickSpacing = isVerticallyOriented ? static_cast<double>(font.getHeight() + 2) : static_cast<double>(mMaximumStringWidth + textPadding * 2);
    auto const sizeRatio = size / useableRangeLength;
    
    // Gets the best range interval between each tick
    auto const continuousNumTicks = std::max(1.0, std::floor(static_cast<double>(size) / minTickSpacing));
    auto const continuousIntervalValue = useableRangeLength / continuousNumTicks;
    auto const intervalValuePower = std::ceil(std::log(continuousIntervalValue) / std::log(mTickPowerInterval));
    auto const discreteIntervalValueNonDivided = std::pow(mTickPowerInterval, intervalValuePower);
    
    // Convert tick reference value into useable range
    auto const useableTickReferenceValue = mFromZoomRange(mTickReferenceValue);
    
    auto discreteIntervalValue = discreteIntervalValueNonDivided;
    if(mTickPowerDivisionFactor > 1.0)
    {
        while(discreteIntervalValue / mTickPowerDivisionFactor * sizeRatio > minTickSpacing)
        {
            discreteIntervalValue /= mTickPowerDivisionFactor;
        }
    }
    
    // Gets the first value and the number of ticks to display
    auto const firstValue = std::floor((useableRange.getStart() - useableTickReferenceValue) / discreteIntervalValue) * discreteIntervalValue + useableTickReferenceValue;
    auto const discreteNumTick = static_cast<size_t>(std::ceil(useableRangeLength / discreteIntervalValue)) + 1;
    
    // Gets the ticks width and text width
    auto const primaryTickSpacing = discreteIntervalValue * static_cast<double>(mPrimaryTickInterval);
    auto const maxTickLength = isVerticallyOriented ? getWidth() - 2 : getHeight() - 2;
    auto const primaryTickLength = static_cast<float>(std::round(maxTickLength * 0.5));
    auto const secondaryTickLength = static_cast<float>(std::round(maxTickLength * 0.25));
    
    g.setFont(font);
    auto const primaryTickEpsilon = visibleRange.getLength() / std::max(static_cast<double>(isVerticallyOriented ? getHeight() - 2 : getWidth() - 2), 1.0);
    for(size_t tickIndex = 0; tickIndex <= discreteNumTick; ++tickIndex)
    {
        auto const currentValue = firstValue + static_cast<double>(tickIndex) * discreteIntervalValue;
        auto const isPrimaryTick = std::abs(std::remainder(currentValue - useableTickReferenceValue, primaryTickSpacing)) < primaryTickEpsilon;
        
        auto const tickLengh = isPrimaryTick ? primaryTickLength : secondaryTickLength;
        auto const position = isVerticallyOriented ? static_cast<float>(size - std::round((currentValue - useableRange.getStart()) * sizeRatio) + 0.5) : static_cast<float>(std::round((currentValue - useableRange.getStart()) * sizeRatio) + 0.5);
        g.setColour(findColour(tickColourId));
        
        if(isVerticallyOriented)
        {
            g.drawLine(1.f, position, tickLengh, position);
        }
        else
        {
            g.drawLine(position, 0, position, tickLengh);
        }

        if(isPrimaryTick && position < size + 1.f)
        {
            g.setColour(findColour(textColourId));
            
            auto const displayValue = mToZoomRange(currentValue);
            juce::String const valueText = (mGetValueAsString != nullptr) ? mGetValueAsString(displayValue) : juce::String(displayValue);
            
            if(isVerticallyOriented)
            {
                if(font.getStringWidth(valueText) < getWidth() - 1)
                {
                    g.drawSingleLineText(valueText, 1, static_cast<int>(std::ceil(position - static_cast<float>(font.getDescent()))), juce::Justification::left);
                }
            }
            else
            {
                g.drawSingleLineText(valueText, static_cast<int>(position + textPadding), 10, juce::Justification::left);
            }
        }
    }
    
    juce::Range<double> const useableVisibleRange { mFromZoomRange(visibleRange.getStart()), mFromZoomRange(visibleRange.getEnd())};
    
    if(mZooming)
    {
        auto const anchorPos = static_cast<int>((mAnchor - useableVisibleRange.getStart()) / useableVisibleRange.getLength() * size);
        
        g.setColour(findColour(anchorColourId, true));
        
        if(isVerticallyOriented)
        {
            g.drawHorizontalLine(size - anchorPos, 0, getWidth());
        }
        else
        {
            g.drawVerticalLine(anchorPos, 0, getHeight());
        }
    }
    
    if(mNavigationMode == NavigationMode::select && !mSelectedValueRange.isEmpty())
    {
        // Convert value / zoom range to pixel space
        juce::Range<double> const usableValueRange { mFromZoomRange(mSelectedValueRange.getStart()), mFromZoomRange(mSelectedValueRange.getEnd())};
        
        auto const pixelStart = static_cast<int>((usableValueRange.getStart() - useableVisibleRange.getStart()) /  useableVisibleRange.getLength() * size);
        auto const pixelEnd = static_cast<int>((usableValueRange.getEnd() - useableVisibleRange.getStart()) /  useableVisibleRange.getLength() * size);
        
        juce::Rectangle<int> selectionRect;
        if(isVerticallyOriented)
        {
            selectionRect = { 0, size - pixelEnd, getWidth(), pixelEnd - pixelStart };
        }
        else
        {
            selectionRect = { pixelStart, 0, pixelEnd - pixelStart, getHeight() };
        }
        
        g.setColour(findColour(selectionColourId).withMultipliedAlpha(0.1f));
        g.fillRect(selectionRect);
        g.setColour(findColour(selectionColourId));
        g.drawRect(selectionRect);
    }
}

ANALYSE_FILE_END
