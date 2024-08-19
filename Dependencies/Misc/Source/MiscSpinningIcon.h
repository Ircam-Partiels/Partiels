#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class SpinningIcon
: public juce::Component
, public juce::SettableTooltipClient
, private juce::Timer
{
public:
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000600
        , foregroundColourId
    };
    // clang-format on

    SpinningIcon(juce::Image const& spinningImage, juce::Image const& inactiveImage = {});
    ~SpinningIcon() override = default;

    bool isActive() const;
    void setActive(bool state);
    void setSpinningImage(juce::Image const& spinningImage);
    void setInactiveImage(juce::Image const& inactiveImage);

    // juce::Component
    void paint(juce::Graphics& g) override;

private:
    // juce::Timer
    void timerCallback() override;

    juce::Image mImageActive;
    juce::Image mImageInactive;
    float mRotation = 0.0f;
};

MISC_FILE_END
