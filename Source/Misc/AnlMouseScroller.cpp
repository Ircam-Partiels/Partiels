#include "AnlMouseScroller.h"

ANALYSE_FILE_BEGIN

MouseScroller::MouseScroller(optional_ref<Zoom::Accessor> horizontalZoomAccessor, optional_ref<Zoom::Accessor> verticalZoomAccessor, optional_ref<Transport::Accessor> transportAccessor)
: mHorizontalAccessor(horizontalZoomAccessor)
, mVerticalAccessor(verticalZoomAccessor)
, mTransportAccessor(transportAccessor)
{
}

void MouseScroller::setAccessors(optional_ref<Zoom::Accessor> horizontalZoomAccessor, optional_ref<Zoom::Accessor> verticalZoomAccessor, optional_ref<Transport::Accessor> transportAccessor)
{
    mHorizontalAccessor = horizontalZoomAccessor;
    mVerticalAccessor = verticalZoomAccessor;
    mTransportAccessor = transportAccessor;
}

void MouseScroller::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    mScrollHelper.mouseWheelMove(wheel, [&](ScrollHelper::Orientation orientation)
                                 {
                                     mScrollOrientation = orientation;
                                     mScrollModifiers = event.mods;
                                 });
    if(mScrollModifiers.isCtrlDown() && mVerticalAccessor.has_value())
    {
        auto& acsr = mVerticalAccessor.value().get();
        auto const delta = mScrollOrientation == ScrollHelper::Orientation::vertical ? wheel.deltaY : wheel.deltaX;
        if(mScrollModifiers.isShiftDown())
        {
            auto const visibleRange = acsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(delta) * visibleRange.getLength();
            acsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange + offset, NotificationType::synchronous);
        }
        else
        {
            auto const anchor = Zoom::Tools::getScaledValueFromHeight(acsr, *this, event.y);
            Zoom::Tools::zoomIn(acsr, static_cast<double>(delta), anchor, NotificationType::synchronous);
        }
    }
    else if(!mScrollModifiers.isCtrlDown() && mHorizontalAccessor.has_value())
    {
        auto& acsr = mHorizontalAccessor.value().get();
        if((mScrollModifiers.isShiftDown() && mScrollOrientation == ScrollHelper::vertical) || mScrollOrientation == ScrollHelper::horizontal)
        {
            auto const visibleRange = acsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const delta = mScrollModifiers.isShiftDown() ? static_cast<double>(wheel.deltaY) : static_cast<double>(wheel.deltaX);
            acsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - delta * visibleRange.getLength(), NotificationType::synchronous);
        }
        else
        {
            auto const amount = static_cast<double>(wheel.deltaY);
            if(mTransportAccessor.has_value())
            {
                auto const anchor = mTransportAccessor.value().get().getAttr<Transport::AttrType::startPlayhead>();
                Zoom::Tools::zoomIn(acsr, amount, anchor, NotificationType::synchronous);
            }
            else
            {
                auto const anchor = Zoom::Tools::getScaledValueFromWidth(acsr, *this, event.x);
                Zoom::Tools::zoomIn(acsr, amount, anchor, NotificationType::synchronous);
            }
        }
    }
}

void MouseScroller::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / -5.0;
    if(event.mods.isCtrlDown() && mVerticalAccessor.has_value())
    {
        auto& acsr = mVerticalAccessor.value().get();
        auto const anchor = Zoom::Tools::getScaledValueFromWidth(acsr, *this, event.y);
        Zoom::Tools::zoomIn(acsr, amount, anchor, NotificationType::synchronous);
    }
    else if(!event.mods.isCtrlDown() && mHorizontalAccessor.has_value())
    {
        auto& acsr = mHorizontalAccessor.value().get();
        auto const anchor = Zoom::Tools::getScaledValueFromWidth(acsr, *this, event.x);
        Zoom::Tools::zoomIn(acsr, amount, anchor, NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
