#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class Icon
: public juce::Button
{
public:
    // clang-format off
    enum ColourIds : int
    {
          normalColourId = 0x2000500
        , overColourId
        , downColourId
    };
    // clang-format on

    Icon(juce::Image const image, juce::Image const onImage = {});
    ~Icon() override = default;

    void setImages(juce::Image const image, juce::Image const onImage = {});

    juce::ModifierKeys getModifierKeys() const;

    // juce::Button
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

protected:
    // juce::Button
    using juce::Button::clicked;
    void clicked(juce::ModifierKeys const& keys) override;

private:
    juce::Image mImage;
    juce::Image mOnImage;
    juce::ModifierKeys mKeys;
};

MISC_FILE_END
