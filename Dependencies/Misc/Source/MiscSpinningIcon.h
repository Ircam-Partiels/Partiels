#pragma once

#include "MiscScalableImage.h"

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

    SpinningIcon(ScalableImage const& spinningImage, ScalableImage const& inactiveImage = {});
    ~SpinningIcon() override = default;

    bool isActive() const;
    void setActive(bool state);
    void setSpinningImage(ScalableImage const& spinningImage);
    void setInactiveImage(ScalableImage const& inactiveImage);

    // juce::Component
    void paint(juce::Graphics& g) override;

private:
    // juce::Timer
    void timerCallback() override;

    ScalableImage mImageActive;
    ScalableImage mImageInactive;
    float mRotation = 0.0f;
};

MISC_FILE_END
