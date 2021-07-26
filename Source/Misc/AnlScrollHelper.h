#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ScrollHelper
: private juce::Timer
{
public:
    // clang-format off
    enum Orientation : bool
    {
          vertical
        , horizontal
    };
    // clang-format on

    ScrollHelper() = default;
    ~ScrollHelper() override = default;

    Orientation mouseWheelMove(juce::MouseWheelDetails const& wheel);

private:
    // juce::Timer
    void timerCallback() override;

    Orientation mOrientation = Orientation::vertical;
    bool mCanUpdateOrientation{true};
};

ANALYSE_FILE_END
