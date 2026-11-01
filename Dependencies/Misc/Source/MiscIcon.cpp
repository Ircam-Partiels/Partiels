#include "MiscIcon.h"
#include "MiscLookAndFeel.h"

MISC_FILE_BEGIN

Icon::Icon(ScalableImage image, ScalableImage onImage)
: juce::Button("Icon")
, mVBlankAttachment(this, [this](double timestamp)
                    {
                        // The ratio moves toward the target at a constant speed so a partial transition takes a partial duration
                        auto constexpr animationDuration = 0.12;
                        auto const step = std::max(static_cast<float>((timestamp - mLastTimestamp) / animationDuration), 0.0f);
                        mLastTimestamp = timestamp;
                        auto const ratio = std::clamp(mOverTarget, mOverRatio - step, mOverRatio + step);
                        if(std::abs(ratio - mOverRatio) > 0.0f)
                        {
                            mOverRatio = ratio;
                            repaint();
                        }
                    })
{
    setImages(std::move(image), std::move(onImage));
}

void Icon::setImages(ScalableImage image, ScalableImage onImage)
{
    mImage = std::move(image);
    mOnImage = std::move(onImage);
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
    auto const area = getLocalBounds().reduced(mPadding);
    auto const scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto& scalableImage = isToggled && hasOnImage ? mOnImage : mImage;
    auto const image = scalableImage.getImage(juce::roundToInt(static_cast<float>(area.getWidth()) * scale), juce::roundToInt(static_cast<float>(area.getHeight()) * scale));
    if(!image.isValid())
    {
        return;
    }

    if(!isEnabled)
    {
        g.beginTransparencyLayer(0.4f);
    }

    auto const colours = [&, this]()
    {
        // Only the transition between the normal and the mouse over states is animated
        if(shouldDrawButtonAsDown || !isEnabled)
        {
            return getColours(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }

        auto const interpolateColours = [](juce::Colour start, juce::Colour end, float ratio)
        {
            // A transparent colour takes the colour of the other one so the transition does not go through black
            if(start.isTransparent())
            {
                start = end.withAlpha(0.0f);
            }
            if(end.isTransparent())
            {
                end = start.withAlpha(0.0f);
            }
            return start.interpolatedWith(end, ratio);
        };

        auto const normalColours = getColours(false, false);
        auto const overColours = getColours(true, false);
        return Colours{interpolateColours(normalColours.background, overColours.background, mOverRatio), interpolateColours(normalColours.foreground, overColours.foreground, mOverRatio)};
    }();

    auto const bounds = getLocalBounds().toFloat();
    auto const cornerSize = std::min(bounds.getWidth(), bounds.getHeight()) * 0.2f;
    if(!colours.background.isTransparent())
    {
        g.setColour(colours.background);
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    auto const transform = [&]()
    {
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

    auto const& colour = colours.foreground;
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

Icon::Colours Icon::getColours(bool isHighlighted, bool isDown) const
{
    auto const hasOnImage = mOnImage.isValid();
    auto const background = [&, this]()
    {
        if(mPadding <= 0)
        {
            return juce::Colours::transparentBlack;
        }
        // If the icon has an image for the toggle state on, the state is already visible so only the mouse down state is highlighted
        auto const isOn = isDown || (getToggleState() && !hasOnImage);
        if(isOn)
        {
            auto const onColour = findColour(Icon::ColourIds::onBackgroundColourId);
            return isHighlighted && !isDown ? onColour.interpolatedWith(findColour(Icon::ColourIds::overColourId), 0.2f) : onColour;
        }
        return isHighlighted ? findColour(Icon::ColourIds::overBackgroundColourId) : juce::Colours::transparentBlack;
    }();

    auto const foreground = [&, this]()
    {
        if(hasOnImage)
        {
            return isHighlighted ? findColour(Icon::ColourIds::overColourId) : findColour(Icon::ColourIds::normalColourId);
        }
        return isDown || getToggleState() ? findColour(Icon::ColourIds::downColourId) : (isHighlighted ? findColour(Icon::ColourIds::overColourId) : findColour(Icon::ColourIds::normalColourId));
    }();
    return {background, foreground};
}

void Icon::mouseEnter(juce::MouseEvent const& event)
{
    mOverTarget = 1.0f;
    juce::Button::mouseEnter(event);
}

void Icon::mouseExit(juce::MouseEvent const& event)
{
    mOverTarget = 0.0f;
    juce::Button::mouseExit(event);
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
