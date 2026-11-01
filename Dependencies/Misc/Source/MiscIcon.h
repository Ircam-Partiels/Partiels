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
        , overBackgroundColourId
        , onBackgroundColourId
        , focusColourId
    };
    // clang-format on

    Icon(juce::Image const image, juce::Image const onImage = {});
    ~Icon() override = default;

    void setImages(juce::Image const image, juce::Image const onImage = {});

    //! @brief Sets the space between the bounds of the icon and its image.
    //! @details The background highlights (mouse over, toggle state on and mouse down) are only drawn if the padding is greater than zero.
    //! @param padding The padding in pixels.
    void setPadding(int padding);

    //! @brief Gets the space between the bounds of the icon and its image.
    //! @return The padding in pixels.
    int getPadding() const;

    juce::ModifierKeys getModifierKeys() const;

    // juce::Button
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

protected:
    // juce::Button
    using juce::Button::clicked;
    void clicked(juce::ModifierKeys const& keys) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;

private:
    juce::Image mImage;
    juce::Image mOnImage;
    juce::ModifierKeys mKeys;
    int mPadding{0};
    bool mShowFocus{false};
};

MISC_FILE_END
