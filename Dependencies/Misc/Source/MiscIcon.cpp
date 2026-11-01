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

void Icon::setPadding(int padding)
{
    padding = std::max(padding, 0);
    if(mPadding != padding)
    {
        mPadding = padding;
        repaint();
    }
}

int Icon::getPadding() const
{
    return mPadding;
}

juce::ModifierKeys Icon::getModifierKeys() const
{
    return mKeys;
}

void Icon::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto const isEnabled = juce::Button::isEnabled();
    if(!isEnabled)
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

    if(!isEnabled)
    {
        g.beginTransparencyLayer(0.4f);
    }

    auto const bounds = getLocalBounds().toFloat();
    auto const cornerSize = std::min(bounds.getWidth(), bounds.getHeight()) * 0.2f;
    if(mPadding > 0)
    {
        // If the icon has an image for the toggle state on, the state is already visible so only the mouse down state is highlighted
        auto const isOn = shouldDrawButtonAsDown || (getToggleState() && !hasOnImage);
        auto const backgroundColour = [&, this]()
        {
            if(isOn)
            {
                auto const onColour = findColour(Icon::ColourIds::onBackgroundColourId);
                return shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown ? onColour.interpolatedWith(findColour(Icon::ColourIds::overColourId), 0.2f) : onColour;
            }
            return shouldDrawButtonAsHighlighted ? findColour(Icon::ColourIds::overBackgroundColourId) : juce::Colours::transparentBlack;
        }();
        if(!backgroundColour.isTransparent())
        {
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(bounds, cornerSize);
        }
    }

    auto const transform = [&]()
    {
        auto const area = getLocalBounds().reduced(mPadding);
        auto const iw = image.getWidth();
        auto const ih = image.getHeight();
        auto const lw = area.getWidth();
        auto const lh = area.getHeight();

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
        auto const targetBounds = juce::Rectangle<int>(area.getX() + (lw - newW) / 2, area.getY() + (lh - newH) / 2, newW, newH).toFloat();
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
        g.setOpacity(1.0f);
        g.drawImageTransformed(image, transform, false);
    }

    if(!colour.isTransparent())
    {
        g.setColour(colour);
        g.drawImageTransformed(image, transform, true);
    }

    if(mShowFocus && hasKeyboardFocus(false))
    {
        g.setColour(findColour(Icon::ColourIds::focusColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    }

    if(!isEnabled)
    {
        g.endTransparencyLayer();
    }
}

void Icon::clicked(juce::ModifierKeys const& keys)
{
    mKeys = keys;
    juce::Button::clicked(keys);
}

void Icon::focusGained(juce::Component::FocusChangeType cause)
{
    // The focus outline is only drawn when the focus is moved with the keyboard
    mShowFocus = cause == juce::Component::FocusChangeType::focusChangedByTabKey;
    juce::Button::focusGained(cause);
}

void Icon::focusLost(juce::Component::FocusChangeType cause)
{
    mShowFocus = false;
    juce::Button::focusLost(cause);
}

MISC_FILE_END
