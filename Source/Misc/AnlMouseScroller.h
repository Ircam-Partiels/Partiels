#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class MouseScroller
: public juce::Component
{
public:
    MouseScroller(optional_ref<Zoom::Accessor> horizontalZoomAccessor = {}, optional_ref<Zoom::Accessor> verticalZoomAccessor = {}, optional_ref<Transport::Accessor> transportAccessor = {});
    ~MouseScroller() override = default;

    void setAccessors(optional_ref<Zoom::Accessor> horizontalZoomAccessor, optional_ref<Zoom::Accessor> verticalZoomAccessor, optional_ref<Transport::Accessor> transportAccessor);

    // juce::Component
    void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
    void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;

private:
    optional_ref<Zoom::Accessor> mHorizontalAccessor;
    optional_ref<Zoom::Accessor> mVerticalAccessor;
    optional_ref<Transport::Accessor> mTransportAccessor;

    ScrollHelper mScrollHelper;
    juce::ModifierKeys mScrollModifiers;
    ScrollHelper::Orientation mScrollOrientation{ScrollHelper::Orientation::horizontal};
};

ANALYSE_FILE_END
