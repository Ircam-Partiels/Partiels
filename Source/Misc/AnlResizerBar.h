#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class ResizerBar final
: public juce::Component
{
public:
    // clang-format off
    enum ColourIds : int
    {
          activeColourId = 0x2000800
        , inactiveColourId
    };
    // clang-format on

    // clang-format off
    enum Orientation
    {
          vertical
        , horizontal
    };
    // clang-format on

    ResizerBar(Orientation const orientation, bool direction, juce::Range<int> const range = {});
    ~ResizerBar() override = default;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseDrag(juce::MouseEvent const& event) override;
    void mouseUp(juce::MouseEvent const& event) override;
    void enablementChanged() override;

    std::function<void(int)> onMoved = nullptr;

private:
    Orientation const mOrientation;
    bool const mDirection;
    juce::Range<int> const mRange;
    int mSavedPosition;
};

ANALYSE_FILE_END