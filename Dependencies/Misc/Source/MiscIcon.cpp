#include "MiscIcon.h"
#include "MiscLookAndFeel.h"

MISC_FILE_BEGIN

Icon::Icon(juce::Image const image, juce::Image const onImage)
{
    setImages(image, onImage);
}

void Icon::setImages(juce::Image const image, juce::Image const onImage)
{
    mImage = image;
    mOnImage = onImage;
    setClickingTogglesState(getClickingTogglesState() || mOnImage.isValid());
    colourChanged();
}

juce::ModifierKeys Icon::getModifierKeys() const
{
    return mKeys;
}

void Icon::colourChanged()
{
    juce::ImageButton::colourChanged();
    auto const onImage = mOnImage.isValid() ? mOnImage : mImage;
    auto const interImage = mOnImage.isValid() && getToggleState() ? onImage : mImage;
    auto const normalColour = findColour(ColourIds::normalColourId);
    auto const overColour = getToggleState() ? normalColour : findColour(ColourIds::overColourId);
    auto const downColour = findColour(ColourIds::downColourId);
    setImages(false, true, true, mImage, 1.0f, normalColour, interImage, 1.0f, overColour, onImage, 1.0f, downColour);
}

void Icon::parentHierarchyChanged()
{
    juce::ImageButton::parentHierarchyChanged();
    colourChanged();
}

void Icon::buttonStateChanged()
{
    juce::ImageButton::buttonStateChanged();
    if(isToggleable())
    {
        colourChanged();
    }
}

void Icon::clicked(juce::ModifierKeys const& keys)
{
    mKeys = keys;
    juce::Button::clicked(keys);
}

MISC_FILE_END
