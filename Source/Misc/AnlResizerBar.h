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
          activeColourId = 0x4000800
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

    int getSavedPosition() const;
    int getNewPosition() const;

    // juce::Component
    void paint(juce::Graphics& g) override;
    void mouseDown(juce::MouseEvent const& event) override;
    void mouseDrag(juce::MouseEvent const& event) override;
    void mouseUp(juce::MouseEvent const& event) override;
    void enablementChanged() override;

    std::function<void(juce::MouseEvent const& event)> onMouseDown = nullptr;
    std::function<void(juce::MouseEvent const& event)> onMouseDrag = nullptr;
    std::function<void(juce::MouseEvent const& event)> onMouseUp = nullptr;

private:
    Orientation const mOrientation;
    bool const mDirection;
    juce::Range<int> const mRange;
    int mSavedPosition;
    int mNewPosition;
};

ANALYSE_FILE_END
