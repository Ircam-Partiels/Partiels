#include "MiscPropertyComponent.h"

MISC_FILE_BEGIN

PropertyComponentBase::PropertyComponentBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip)
: content(std::move(c))
{
    title.setText(name + ":", juce::NotificationType::dontSendNotification);
    title.setMinimumHorizontalScale(1.f);
    title.setInterceptsMouseClicks(false, false);
    auto const& borderSize = title.getBorderSize();
    title.setBorderSize({borderSize.getTop(), borderSize.getLeft(), borderSize.getBottom(), 0});
    title.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(title);
    addAndMakeVisible(content.get());
    setTooltip(tooltip);
    setSize(200, defaultHeight);
}

void PropertyComponentBase::resized()
{
    auto& titleLookAndFeel = title.getLookAndFeel();
    auto const minimumHorizontalScale = title.getMinimumHorizontalScale();
    auto const font = titleLookAndFeel.getLabelFont(title).withHorizontalScale(minimumHorizontalScale);
    auto const bdSize = titleLookAndFeel.getLabelBorderSize(title);

    auto bounds = getLocalBounds();
    auto const textWidth = juce::GlyphArrangement::getStringWidthInt(font, title.getText());
    auto const textSize = textWidth + bdSize.getLeft() + bdSize.getRight();
    title.setBounds(bounds.removeFromLeft(textSize));
    if(content != nullptr)
    {
        content->setBounds(bounds);
    }
}

void PropertyComponentBase::setTooltip(juce::String const& newTooltip)
{
    juce::SettableTooltipClient::setTooltip(newTooltip);
    title.setTooltip(newTooltip);
    if(auto* entry = dynamic_cast<juce::SettableTooltipClient*>(content.get()))
    {
        entry->setTooltip(newTooltip);
    }
}

PropertyTextButton::PropertyTextButton(juce::String const& name, juce::String const& tooltip, std::function<void(void)> fn)
: PropertyComponent<juce::TextButton>(name, tooltip, nullptr)
{
    title.setVisible(false);
    entry.setButtonText(name);
    entry.onClick = [=]()
    {
        if(fn != nullptr)
        {
            fn();
        }
    };
}

void PropertyTextButton::resized()
{
    entry.setBounds(getLocalBounds().reduced(32, 2));
}

PropertyText::PropertyText(juce::String const& name, juce::String const& tooltip, std::function<void(juce::String)> fn)
: PropertyComponent<juce::Label>(name, tooltip)
{
    entry.setRepaintsOnMouseActivity(true);
    entry.setEditable(true);
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
    entry.setBorderSize({});
    entry.onEditorShow = [&]()
    {
        if(auto* editor = entry.getCurrentTextEditor())
        {
            auto const font = entry.getFont();
            editor->setFont(font);
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())) - 1);
            editor->setBorder(entry.getBorderSize());
            editor->setJustification(entry.getJustificationType());
        }
    };
    entry.onTextChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(entry.getText());
        }
    };
}

PropertyLabel::PropertyLabel(juce::String const& name, juce::String const& tooltip)
: PropertyText(name, tooltip, nullptr)
{
    entry.setEditable(false, false);
}

PropertyNumber::PropertyNumber(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn)
: PropertyComponent<NumberField>(name, tooltip)
{
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    entry.setNumDecimalsDisplayed(interval > 0.0f ? entry.getNumEditedDecimals() : 2);
    entry.setJustificationType(juce::Justification::centredRight);
    entry.setTextValueSuffix(suffix);
    entry.onValueChanged = [=](double value)
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(value));
        }
    };
}

PropertySlider::PropertySlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(void)> startFn, std::function<void(float)> fn, std::function<void(void)> endFn, bool showNumberField)
: PropertyComponent<juce::Slider>(name, tooltip)
{
    entry.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    entry.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval));
    entry.setTextValueSuffix(suffix);
    entry.setScrollWheelEnabled(false);
    entry.onDragStart = [=]()
    {
        if(startFn != nullptr)
        {
            startFn();
        }
    };
    entry.onValueChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(entry.getValue()));
        }
    };
    entry.onDragEnd = [=]()
    {
        if(endFn != nullptr)
        {
            endFn();
        }
    };

    numberField.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    numberField.setNumDecimalsDisplayed(interval > 0.0f ? numberField.getNumEditedDecimals() : 2);
    numberField.setJustificationType(juce::Justification::centredRight);
    numberField.setTextValueSuffix(suffix);
    numberField.setTooltip(tooltip);
    numberField.onValueChanged = [=](double value)
    {
        if(startFn != nullptr)
        {
            startFn();
        }
        if(fn != nullptr)
        {
            fn(static_cast<float>(value));
        }
        if(endFn != nullptr)
        {
            endFn();
        }
    };
    addChildComponent(numberField);
    numberField.setVisible(showNumberField);
    setSize(getWidth(), showNumberField ? defaultHeight * 2 : defaultHeight);
}

void PropertySlider::resized()
{
    auto& titleLookAndFeel = title.getLookAndFeel();
    auto const minimumHorizontalScale = title.getMinimumHorizontalScale();
    auto const font = titleLookAndFeel.getLabelFont(title).withHorizontalScale(minimumHorizontalScale);
    auto const bdSize = titleLookAndFeel.getLabelBorderSize(title);

    auto bounds = getLocalBounds();
    if(numberField.isVisible() && content != nullptr)
    {
        content->setBounds(bounds.removeFromBottom(bounds.getHeight() / 2));
    }
    auto const textWidth = juce::GlyphArrangement::getStringWidthInt(font, title.getText());
    auto const textSize = textWidth + bdSize.getLeft() + bdSize.getRight();
    title.setBounds(bounds.removeFromLeft(textSize));
    if(numberField.isVisible())
    {
        numberField.setBounds(bounds);
    }
    else if(content != nullptr)
    {
        content->setBounds(bounds);
    }
}

void PropertySlider::setTooltip(juce::String const& newTooltip)
{
    PropertyComponentBase::setTitle(newTooltip);
    numberField.setTooltip(newTooltip);
}

PropertyRangeSlider::PropertyRangeSlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(void)> startFn, std::function<void(float, float)> fn, std::function<void(void)> endFn, bool showNumberField)
: PropertyComponent<juce::Slider>(name, tooltip)
{
    entry.setSliderStyle(juce::Slider::SliderStyle::TwoValueHorizontal);
    entry.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval));
    entry.setTextValueSuffix(suffix);
    entry.setScrollWheelEnabled(false);
    entry.onDragStart = [=]()
    {
        if(startFn != nullptr)
        {
            startFn();
        }
    };
    entry.onValueChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(entry.getMinValue()), static_cast<float>(entry.getMaxValue()));
        }
    };
    entry.onDragEnd = [=]()
    {
        if(endFn != nullptr)
        {
            endFn();
        }
    };

    numberFieldLow.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    numberFieldLow.setNumDecimalsDisplayed(interval > 0.0f ? numberFieldLow.getNumEditedDecimals() : 2);
    numberFieldLow.setJustificationType(juce::Justification::centredLeft);
    numberFieldLow.setTextValueSuffix(suffix);
    numberFieldLow.setTooltip(tooltip);
    numberFieldLow.onValueChanged = [=, this](double value)
    {
        if(startFn != nullptr)
        {
            startFn();
        }
        if(fn != nullptr)
        {
            auto const high = std::max(static_cast<float>(entry.getMaxValue()), static_cast<float>(value));
            fn(static_cast<float>(value), high);
        }
        if(endFn != nullptr)
        {
            endFn();
        }
    };
    addChildComponent(numberFieldLow);
    numberFieldLow.setVisible(showNumberField);

    numberFieldHigh.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    numberFieldHigh.setNumDecimalsDisplayed(interval > 0.0f ? numberFieldHigh.getNumEditedDecimals() : 2);
    numberFieldHigh.setJustificationType(juce::Justification::centredRight);
    numberFieldHigh.setTextValueSuffix(suffix);
    numberFieldHigh.setTooltip(tooltip);
    numberFieldHigh.onValueChanged = [=, this](double value)
    {
        if(startFn != nullptr)
        {
            startFn();
        }
        if(fn != nullptr)
        {
            auto const low = std::min(static_cast<float>(entry.getMinValue()), static_cast<float>(value));
            fn(low, static_cast<float>(value));
        }
        if(endFn != nullptr)
        {
            endFn();
        }
    };
    addChildComponent(numberFieldHigh);
    numberFieldHigh.setVisible(showNumberField);

    setSize(getWidth(), showNumberField ? defaultHeight * 2 : defaultHeight);
}

void PropertyRangeSlider::resized()
{
    auto& titleLookAndFeel = title.getLookAndFeel();
    auto const minimumHorizontalScale = title.getMinimumHorizontalScale();
    auto const font = titleLookAndFeel.getLabelFont(title).withHorizontalScale(minimumHorizontalScale);
    auto const bdSize = titleLookAndFeel.getLabelBorderSize(title);

    auto bounds = getLocalBounds();
    if((numberFieldLow.isVisible() || numberFieldHigh.isVisible()) && content != nullptr)
    {
        content->setBounds(bounds.removeFromBottom(bounds.getHeight() / 2));
    }
    auto const width = bounds.getWidth();
    auto const textWidth = juce::GlyphArrangement::getStringWidthInt(font, title.getText());
    auto const textSize = textWidth + bdSize.getLeft() + bdSize.getRight();
    title.setBounds(bounds.removeFromLeft(textSize));
    if(numberFieldLow.isVisible() || numberFieldHigh.isVisible())
    {
        if(textSize < width / 2)
        {
            bounds = bounds.withTrimmedLeft(width / 2 - textSize);
        }
        numberFieldLow.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        numberFieldHigh.setBounds(bounds);
    }
    else if(content != nullptr)
    {
        content->setBounds(bounds);
    }
}

void PropertyRangeSlider::setTooltip(juce::String const& newTooltip)
{
    PropertyComponentBase::setTitle(newTooltip);
    numberFieldLow.setTooltip(newTooltip);
    numberFieldHigh.setTooltip(newTooltip);
}

PropertyToggle::PropertyToggle(juce::String const& name, juce::String const& tooltip, std::function<void(bool)> fn)
: PropertyComponent<juce::ToggleButton>(name, tooltip)
{
    entry.onClick = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(entry.getToggleState());
        }
    };
}

void PropertyToggle::resized()
{
    PropertyComponent<juce::ToggleButton>::resized();
    entry.setBounds(getLocalBounds().removeFromRight(getHeight()).reduced(4));
}

PropertyList::PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn)
: PropertyComponent<juce::ComboBox>(name, tooltip)
{
    juce::StringArray items;
    for(auto const& value : values)
    {
        items.add(juce::String(value) + suffix);
    }
    entry.addItemList(items, 1);
    entry.setJustificationType(juce::Justification::centredRight);
    entry.onChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(entry.getSelectedItemIndex() > 0 ? static_cast<size_t>(entry.getSelectedItemIndex()) : 0_z);
        }
    };
}

PropertyColourButton::PropertyColourButton(juce::String const& name, juce::String const& tooltip, juce::String const& header, std::function<void(juce::Colour)> fn, std::function<void()> onEditorShow, std::function<void()> onEditorHide)
: PropertyComponent<ColourButton>(name, tooltip)
{
    entry.setTitle(header);
    entry.onColourChanged = [=](juce::Colour const& colour)
    {
        if(fn != nullptr)
        {
            fn(colour);
        }
    };
    entry.onColourSelectorShow = onEditorShow;
    entry.onColourSelectorHide = onEditorHide;
}

void PropertyColourButton::resized()
{
    PropertyComponent<ColourButton>::resized();
    entry.setBounds(getLocalBounds().removeFromRight(getHeight()).reduced(4));
}

PropertyHMSmsField::PropertyHMSmsField(juce::String const& name, juce::String const& tooltip, std::function<void(double)> fn)
: PropertyComponent<HMSmsField>(name, tooltip)
{
    entry.setJustificationType(juce::Justification::centredRight);
    entry.onTimeChanged = [=](double value)
    {
        if(fn != nullptr)
        {
            fn(value);
        }
    };
}

void PropertyHMSmsField::resized()
{
    PropertyComponent<HMSmsField>::resized();
    entry.setBounds(getLocalBounds().removeFromRight(120));
}

MISC_FILE_END
