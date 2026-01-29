#include "MiscIcon.h"
#include "MiscLookAndFeel.h"

MISC_FILE_BEGIN

Icon::Icon(juce::Image const image, juce::Image const onImage)
: juce::Button("Icon")
{
    setImages(image, onImage);
}

void Icon::setImages(juce::Image const image, juce::Image const onImage)
{
    mImage = image;
    mOnImage = onImage;
    setClickingTogglesState(getClickingTogglesState() || mOnImage.isValid());
    repaint();
}

juce::ModifierKeys Icon::getModifierKeys() const
{
    return mKeys;
}

void Icon::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    if(!isEnabled())
    {
        shouldDrawButtonAsHighlighted = false;
        shouldDrawButtonAsDown = false;
    }

    auto const isToggled = shouldDrawButtonAsDown || getToggleState();
    auto const hasOnImage = mOnImage.isValid();
    auto const image = isToggled && hasOnImage ? mOnImage : mImage;
    if(!image.isValid())
    {
        return;
    }
    auto const transform = [&, this]
    {
        auto const iw = image.getWidth();
        auto const ih = image.getHeight();
        auto const lw = getWidth();
        auto const lh = getHeight();

        int newW, newH;
        auto const imRatio = static_cast<float>(ih) / static_cast<float>(iw);
        auto const destRatio = static_cast<float>(lh) / static_cast<float>(lw);
        if(imRatio > destRatio)
        {
            newW = static_cast<int>(std::round(static_cast<float>(lh) / imRatio));
            newH = lh;
        }
        else
        {
            newW = lw;
            newH = static_cast<int>(std::round(static_cast<float>(lw) * imRatio));
        }
        auto const targetBounds = juce::Rectangle<int>((lw - newW) / 2, (lh - newH) / 2, newW, newH).toFloat();
        auto const rectanglePlacement = juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit);
        return juce::AffineTransform{rectanglePlacement.getTransformToFit(image.getBounds().toFloat(), targetBounds)};
    }();

    auto const colour = [&, this]()
    {
        if(hasOnImage)
        {
            return shouldDrawButtonAsHighlighted ? findColour(Icon::ColourIds::overColourId) : findColour(Icon::ColourIds::normalColourId);
        }
        return isToggled ? findColour(Icon::ColourIds::downColourId) : (shouldDrawButtonAsHighlighted ? findColour(Icon::ColourIds::overColourId) : findColour(Icon::ColourIds::normalColourId));
    }();

    if(!colour.isOpaque())
    {
        g.setOpacity(isEnabled() ? 1.0f : 0.3f);
        g.drawImageTransformed(image, transform, false);
    }

    if(!colour.isTransparent())
    {
        g.setColour(colour);
        g.drawImageTransformed(image, transform, true);
    }
}

void Icon::clicked(juce::ModifierKeys const& keys)
{
    mKeys = keys;
    juce::Button::clicked(keys);
}

MISC_FILE_END
