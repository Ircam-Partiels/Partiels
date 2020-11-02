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
    
    mReceiver.onSignal = [&](Accessor const& acsr, Signal signal, juce::var value)
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
    
    auto const visibleRange = mAccessor.getModel().range;
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
            auto const translation = static_cast<double>(delta) / static_cast<double>(size) * copy.range.getLength();
            copy.range += translation;
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
            auto const zoomFactor = zoomDistance < 0.0 ? std::pow(1.005, std::abs(zoomDistance)) : 1.0 / std::pow(1.005, std::abs(zoomDistance));
            auto const rangeStart = mAnchor - (mAnchor - mInitialValueRange.getStart()) * zoomFactor;
            auto const rangeEnd = mAnchor + (mInitialValueRange.getEnd() - mAnchor) * zoomFactor;

            auto copy = mAccessor.getModel();
            copy.range = {mToZoomRange(rangeStart), mToZoomRange(rangeEnd)};
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
            copy.range = mSelectedValueRange;
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
    auto fromZoomRange = [this](juce::Range<double> const& range) -> juce::Range<double>
    {
        return {mFromZoomRange(range.getStart()), mFromZoomRange(range.getEnd())};
    };
    
    auto const visibleRange = mAccessor.getModel().range;
    auto const useableRange = fromZoomRange(visibleRange);
    if(useableRange.isEmpty())
    {
        return;
    }
    
    auto const useableRangeLength = useableRange.getLength();
    auto const height = getHeight();
    auto const width = getWidth();
    auto const isVerticallyOriented = mOrientation == Orientation::vertical;
    auto const size = isVerticallyOriented ? height - 1 : width - 1;
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
    auto const primaryTickEpsilon = visibleRange.getLength() / std::max(static_cast<double>(isVerticallyOriented ? height - 2 : width - 2), 1.0);
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
            g.drawVerticalLine(static_cast<int>(std::floor(position)) + 1, height - tickLengh, height);
        }

        if(isPrimaryTick && position < size + 1.f)
        {
            g.setColour(findColour(textColourId));
            
            auto const displayValue = mToZoomRange(currentValue);
            auto const valueText = (mGetValueAsString != nullptr) ? mGetValueAsString(displayValue) : juce::String(displayValue);
            
            if(isVerticallyOriented)
            {
                if(font.getStringWidth(valueText) < width - 1)
                {
                    g.drawSingleLineText(valueText, 1, static_cast<int>(std::ceil(position - static_cast<float>(font.getDescent()))), juce::Justification::left);
                }
            }
            else
            {
                g.drawSingleLineText(valueText, static_cast<int>(position + textPadding), height - 1, juce::Justification::left);
            }
        }
    }
    
    if(mZooming)
    {
        g.setColour(findColour(anchorColourId, true));
        auto const anchor = (mAnchor - useableRange.getStart()) / useableRange.getLength();
        if(isVerticallyOriented)
        {
            auto const anchorPosition = static_cast<int>(static_cast<float>(height) - (1.0f - anchor));
            g.drawHorizontalLine(anchorPosition, 0.0f, static_cast<float>(width));
        }
        else
        {
            auto const anchorPosition = static_cast<int>(anchor * static_cast<float>(width));
            g.drawVerticalLine(anchorPosition, 0.0f, static_cast<float>(height));
        }
    }
    else if(!mSelectedValueRange.isEmpty())
    {
        auto getSelectionRectangle = [&]() -> juce::Rectangle<float>
        {
            auto const usableValueRange = fromZoomRange(mSelectedValueRange);
            auto const start = static_cast<float>((usableValueRange.getStart() - useableRange.getStart()) /  useableRange.getLength());
            auto const end = static_cast<float>((usableValueRange.getEnd() - useableRange.getStart()) /  useableRange.getLength());
            if(isVerticallyOriented)
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
