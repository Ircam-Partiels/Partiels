#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

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

    void mouseWheelMove(juce::MouseWheelDetails const& wheel, std::function<void(Orientation orientation)> updated);

private:
    // juce::Timer
    void timerCallback() override;

    std::optional<Orientation> mOrientation;
};

MISC_FILE_END
