#include "AnlColourSelector.h"

ANALYSE_FILE_BEGIN

ColourSelector::Header::Header(ColourSelector& owner)
: mOwner(owner)
{
    addAndMakeVisible(mLabel);
    mLabel.setFont(juce::Font(14));
    mLabel.setJustificationType(juce::Justification::centred);
    mLabel.setEditable(true);
    mLabel.onEditorShow = [this]
    {
        if(auto* editor = mLabel.getCurrentTextEditor())
        {
            editor->setInputRestrictions(8, "1234567890ABCDEFabcdef");
            editor->setJustification(juce::Justification::centred);
        }
    };

    mLabel.onTextChange = [this]
    {
        mOwner.setCurrentColour(juce::Colour::fromString(mLabel.getText()), juce::NotificationType::sendNotificationSync);
    };
}

void ColourSelector::Header::resized()
{
    mLabel.setBounds(getLocalBounds());
}

void ColourSelector::Header::paint(juce::Graphics& g)
{
    auto const colour = mOwner.getCurrentColour();
    g.fillCheckerBoard(getLocalBounds().toFloat(), 10.0f, 10.0f, juce::Colour(0xffdddddd).overlaidWith(colour), juce::Colour(0xffffffff).overlaidWith(colour));
}

void ColourSelector::Header::update()
{
    auto const colour = mOwner.getCurrentColour();
    auto const textColour = juce::Colours::white.overlaidWith(colour).contrasting();
    mLabel.setColour(juce::Label::textColourId, textColour);
    mLabel.setColour(juce::Label::textWhenEditingColourId, textColour);
    mLabel.setText(colour.toDisplayString(true), juce::NotificationType::dontSendNotification);
    repaint();
}

ColourSelector::HueSelector::HueSelector(ColourSelector& owner)
: mOwner(owner)
{
}

void ColourSelector::HueSelector::paint(juce::Graphics& g)
{
    auto const height = static_cast<float>(getHeight());
    auto const width = static_cast<float>(getWidth());

    juce::ColourGradient cg;
    cg.isRadial = false;
    cg.point1.setXY(0.0f, 0.0f);
    cg.point2.setXY(width, 0.0f);

    for(float i = 0.0f; i <= 1.0f; i += 0.02f)
    {
        cg.addColour(i, juce::Colour(i, 1.0f, 1.0f, 1.0f));
    }

    g.setGradientFill(cg);
    g.fillRect(getLocalBounds());

    auto const hsba = mOwner.getHSBA();
    auto const area = juce::Rectangle<float>(hsba[0] * width - height / 2.0f, 0, height, height);
    g.setColour(juce::Colours::black);
    g.drawEllipse(area.reduced(1.0f), 1.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(area.reduced(2.0f), 1.0f);
}

void ColourSelector::HueSelector::mouseDown(juce::MouseEvent const& event)
{
    mouseDrag(event);
}

void ColourSelector::HueSelector::mouseDrag(juce::MouseEvent const& event)
{
    auto hsba = mOwner.getHSBA();
    hsba[0] = std::min(std::max(static_cast<float>(event.x) / static_cast<float>(getWidth()), 0.0f), 1.0f);
    mOwner.setHSBA(hsba, juce::NotificationType::sendNotificationSync);
}

ColourSelector::SatBrightSelector::SatBrightSelector(ColourSelector& owner)
: mOwner(owner)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void ColourSelector::SatBrightSelector::paint(juce::Graphics& g)
{
    auto const hsba = mOwner.getHSBA();
    if(std::abs(mHue - hsba[0]) > std::numeric_limits<float>::epsilon() || mImage.isNull())
    {
        auto const width = getWidth() / 2;
        auto const height = getHeight() / 2;
        mImage = juce::Image(juce::Image::RGB, width, height, false);
        juce::Image::BitmapData pixels(mImage, juce::Image::BitmapData::writeOnly);
        auto const hue = hsba[0];
        for(int y = 0; y < height; ++y)
        {
            auto const brightness = 1.0f - static_cast<float>(y) / static_cast<float>(height);
            for(int x = 0; x < width; ++x)
            {
                auto const saturation = static_cast<float>(x) / static_cast<float>(width);
                pixels.setPixelColour(x, y, juce::Colour(hue, saturation, brightness, 1.0f));
            }
        }
        mHue = hsba[0];
    }

    g.setOpacity(1.0f);
    g.drawImageTransformed(mImage, juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit).getTransformToFit(mImage.getBounds().toFloat(), getLocalBounds().toFloat()), false);

    auto const height = static_cast<float>(getHeight());
    auto const width = static_cast<float>(getWidth());

    auto const area = juce::Rectangle<float>(hsba[1] * width - 16.0f / 2.0f, (1.0f - hsba[2]) * height - 16.0f / 2.0f, 16, 16);
    g.setColour(juce::Colours::black);
    g.drawEllipse(area.reduced(1.0f), 1.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(area.reduced(2.0f), 1.0f);
}

void ColourSelector::SatBrightSelector::resized()
{
    mImage = {};
}

void ColourSelector::SatBrightSelector::mouseDown(juce::MouseEvent const& event)
{
    mouseDrag(event);
}

void ColourSelector::SatBrightSelector::mouseDrag(juce::MouseEvent const& event)
{
    auto hsba = mOwner.getHSBA();
    hsba[1] = std::min(std::max(static_cast<float>(event.x) / static_cast<float>(getWidth()), 0.0f), 1.0f);
    hsba[2] = std::min(std::max(1.0f - static_cast<float>(event.y) / static_cast<float>(getHeight()), 0.0f), 1.0f);
    mOwner.setHSBA(hsba, juce::NotificationType::sendNotificationSync);
}

ColourSelector::ComponentSlider::ComponentSlider(ColourSelector& owner, juce::String label, size_t index)
: mOwner(owner)
, mIndex(index)
{
    mLabel.setEditable(false);
    mLabel.setFont(juce::Font(14));
    mLabel.setJustificationType(juce::Justification::centredLeft);
    mLabel.setText(label, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(mLabel);

    mValue.setFont(juce::Font(14));
    mValue.setJustificationType(juce::Justification::centred);
    mValue.setEditable(true);
    mValue.onEditorShow = [this]
    {
        if(auto* editor = mValue.getCurrentTextEditor())
        {
            editor->setInputRestrictions(3, "1234567890");
            editor->setJustification(juce::Justification::centred);
        }
    };

    auto updateValue = [this](juce::uint8 value)
    {
        auto const colour = mOwner.getCurrentColour();
        std::array<juce::uint8, 4> rgba;
        rgba[0] = colour.getRed();
        rgba[1] = colour.getGreen();
        rgba[2] = colour.getBlue();
        rgba[3] = colour.getAlpha();
        rgba[mIndex] = value;
        auto const newColour = juce::Colour::fromRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
        auto const preserveHue = newColour == juce::Colours::white || newColour == juce::Colours::black;
        auto hsba = mOwner.getHSBA();
        hsba[0] = preserveHue ? hsba[0] : newColour.getHue();
        hsba[1] = newColour.getSaturation();
        hsba[2] = newColour.getBrightness();
        hsba[3] = newColour.getFloatAlpha();
        mOwner.setHSBA(hsba, juce::NotificationType::sendNotificationSync);
    };

    mValue.onTextChange = [=]
    {
        updateValue(static_cast<juce::uint8>(std::min(std::max(mValue.getText().getIntValue(), 0), 255)));
    };
    addAndMakeVisible(mValue);

    mSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    mSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    mSlider.setRange({0.0, 255.0}, 1.0);
    mSlider.onValueChange = [=]()
    {
        updateValue(static_cast<juce::uint8>(std::min(std::max(mSlider.getValue(), 0.0), 255.0)));
    };
    addAndMakeVisible(mSlider);
}

void ColourSelector::ComponentSlider::resized()
{
    auto bounds = getLocalBounds();
    mLabel.setBounds(bounds.removeFromLeft(42));
    bounds.removeFromLeft(2);
    mValue.setBounds(bounds.removeFromLeft(42));
    bounds.removeFromLeft(2);
    mSlider.setBounds(bounds);
}

void ColourSelector::ComponentSlider::update()
{
    auto const colour = mOwner.getCurrentColour();
    std::array<juce::uint8, 4> rgba;
    rgba[0] = colour.getRed();
    rgba[1] = colour.getGreen();
    rgba[2] = colour.getBlue();
    rgba[3] = colour.getAlpha();
    mValue.setText(juce::String(rgba[mIndex]), juce::NotificationType::dontSendNotification);
    mSlider.setValue(static_cast<double>(rgba[mIndex]), juce::NotificationType::dontSendNotification);
}

ColourSelector::ColourSelector()
{
    addAndMakeVisible(mHeader);
    addAndMakeVisible(mHueSelector);
    addAndMakeVisible(mSatBrightSelector);
    addAndMakeVisible(mRedSlider);
    addAndMakeVisible(mGreenSlider);
    addAndMakeVisible(mBlueSlider);
    addAndMakeVisible(mAlphaSlider);
    setSize(364, 242);
}

juce::Colour ColourSelector::getCurrentColour() const
{
    return juce::Colour(mHSBA[0], mHSBA[1], mHSBA[2], mHSBA[3]);
}

void ColourSelector::setCurrentColour(juce::Colour newColour, juce::NotificationType notificationType)
{
    auto const hue = newColour.getHue();
    auto const saturation = newColour.getSaturation();
    auto const brightness = newColour.getBrightness();
    auto const alpha = newColour.getFloatAlpha();
    setHSBA({hue, saturation, brightness, alpha}, notificationType);
}

std::array<float, 4> ColourSelector::getHSBA() const
{
    return mHSBA;
}

void ColourSelector::setHSBA(std::array<float, 4> const& hsba, juce::NotificationType notificationType)
{
    if(!std::equal(mHSBA.cbegin(), mHSBA.cend(), hsba.cbegin(), [](auto const& lhs, auto const& rhs)
                   {
                       return std::abs(lhs - rhs) < std::numeric_limits<double>::epsilon();
                   }))
    {
        mHSBA = hsba;
        mHueSelector.repaint();
        mSatBrightSelector.repaint();
        mHeader.update();
        mRedSlider.update();
        mGreenSlider.update();
        mBlueSlider.update();
        mAlphaSlider.update();

        if(notificationType == juce::NotificationType::sendNotificationAsync)
        {
            juce::WeakReference<juce::Component> target(this);
            juce::MessageManager::callAsync([=]()
                                            {
                                                if(target.get() != nullptr && onColourChanged != nullptr)
                                                {
                                                    onColourChanged(getCurrentColour());
                                                }
                                            });
        }
        else if(notificationType != juce::NotificationType::dontSendNotification)
        {
            if(onColourChanged != nullptr)
            {
                onColourChanged(getCurrentColour());
            }
        }
    }
}

void ColourSelector::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    mHeader.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(2);
    mHueSelector.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(2);
    mSatBrightSelector.setBounds(bounds.removeFromTop(128));
    bounds.removeFromTop(2);
    mRedSlider.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(2);
    mGreenSlider.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(2);
    mBlueSlider.setBounds(bounds.removeFromTop(16));
    bounds.removeFromTop(2);
    mAlphaSlider.setBounds(bounds.removeFromTop(16));
}

ColourButton::ColourButton()
: juce::Button("ColourButton")
{
    setClickingTogglesState(false);
}

juce::Colour ColourButton::getCurrentColour() const
{
    return mColour;
}

void ColourButton::setCurrentColour(juce::Colour const& newColour, juce::NotificationType notificationType)
{
    if(newColour != mColour)
    {
        mColour = newColour;
        repaint();

        if(notificationType == juce::NotificationType::sendNotificationAsync)
        {
            juce::WeakReference<juce::Component> target(this);
            juce::MessageManager::callAsync([=, this]
                                            {
                                                if(target.get() != nullptr && onColourChanged != nullptr)
                                                {
                                                    onColourChanged(getCurrentColour());
                                                }
                                            });
        }
        else if(notificationType != juce::NotificationType::dontSendNotification)
        {
            if(onColourChanged != nullptr)
            {
                onColourChanged(getCurrentColour());
            }
        }
    }
}

bool ColourButton::isColourSelectorVisible() const
{
    return mIsColourSelectorVisible;
}

void ColourButton::mouseDown(juce::MouseEvent const& event)
{
    juce::Button::mouseDown(event);
    juce::ignoreUnused(event);
}

void ColourButton::mouseDrag(juce::MouseEvent const& event)
{
    if(!event.source.isLongPressOrDrag())
    {
        juce::Button::mouseDrag(event);
        return;
    }

    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive())
    {
        juce::Image snapshot(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        paintEntireComponent(g, false);
        g.endTransparencyLayer();

        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging({}, this, snapshot, true, &p, &event.source);
    }
}

void ColourButton::mouseUp(juce::MouseEvent const& event)
{
    if(!event.source.isLongPressOrDrag())
    {
        juce::Button::mouseUp(event);
    }
}

void ColourButton::clicked()
{
    anlWeakAssert(mIsColourSelectorVisible == false);
    if(mIsColourSelectorVisible == true)
    {
        return;
    }
    auto colourSelector = std::make_unique<ColourSelector>();
    if(colourSelector == nullptr)
    {
        return;
    }
    colourSelector->setCurrentColour(mColour, juce::NotificationType::dontSendNotification);
    colourSelector->onColourChanged = [&](juce::Colour const& colour)
    {
        setCurrentColour(colour, juce::NotificationType::sendNotificationSync);
    };

    mIsColourSelectorVisible = true;
    auto& box = juce::CallOutBox::launchAsynchronously(std::move(colourSelector), getScreenBounds(), nullptr);
    box.setArrowSize(0.0f);
    box.resized();
    box.addComponentListener(this);
    if(onColourSelectorShow != nullptr)
    {
        onColourSelectorShow();
    }
}

void ColourButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto constexpr cornerSize = 4.0f;
    auto constexpr thickness = 1.0f;
    auto constexpr cellSize = 8;
    auto const bounds = getLocalBounds().toFloat().reduced(thickness);
    auto const colour = mDraggedColour != nullptr ? mDraggedColour->mColour : mColour;

    g.saveState();
    juce::Path p;
    p.addRoundedRectangle(bounds, cornerSize);
    g.reduceClipRegion(p);
    g.fillAll(juce::Colours::lightgrey);
    g.setColour(juce::Colours::white);
    for(auto i = 0; i < getWidth(); i += cellSize)
    {
        auto const iIsOdd = (i / (cellSize)) % 2;
        for(auto j = 0; j < getHeight(); j += cellSize)
        {
            auto const jIsOdd = (j / (cellSize)) % 2;
            if(iIsOdd != jIsOdd)
            {
                g.fillRect(i, j, cellSize, cellSize);
            }
        }
    }
    g.setColour(colour);
    g.fillAll(colour);
    g.restoreState();

    if(shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour(findColour(ColourIds::borderOnColourId));
    }
    else
    {
        g.setColour(findColour(ColourIds::borderOffColourId));
    }
    g.drawRoundedRectangle(bounds, cornerSize, thickness);
}

bool ColourButton::isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    if(source == nullptr || source == this)
    {
        return false;
    }
    return true;
}

void ColourButton::itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    mDraggedColour = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    anlWeakAssert(mDraggedColour.getComponent() != nullptr);
    if(mDraggedColour.getComponent() == nullptr)
    {
        mDraggedColour = nullptr;
        return;
    }
    repaint();
}

void ColourButton::itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    juce::ignoreUnused(dragSourceDetails);
    mDraggedColour = nullptr;
    repaint();
}

void ColourButton::itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    mDraggedColour = dynamic_cast<ColourButton*>(dragSourceDetails.sourceComponent.get());
    anlWeakAssert(mDraggedColour.getComponent() != nullptr);
    if(mDraggedColour == nullptr)
    {
        mDraggedColour = nullptr;
        return;
    }

    setCurrentColour(mDraggedColour->mColour, juce::NotificationType::sendNotificationSync);
    itemDragExit(dragSourceDetails);
}

void ColourButton::componentBeingDeleted(juce::Component& component)
{
    juce::ignoreUnused(component);
    anlWeakAssert(mIsColourSelectorVisible == true);
    if(mIsColourSelectorVisible == false)
    {
        return;
    }
    mIsColourSelectorVisible = false;
    if(onColourSelectorHide != nullptr)
    {
        onColourSelectorHide();
    }
}

ANALYSE_FILE_END
