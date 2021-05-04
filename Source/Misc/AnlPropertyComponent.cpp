#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

PropertyComponentBase::PropertyComponentBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip)
: content(std::move(c))
{
    title.setText(name + ":", juce::NotificationType::dontSendNotification);
    title.setTooltip(tooltip);
    title.setMinimumHorizontalScale(1.f);
    title.setInterceptsMouseClicks(false, false);
    auto const& borderSize = title.getBorderSize();
    title.setBorderSize({borderSize.getTop(), borderSize.getLeft(), borderSize.getBottom(), 0});

    addAndMakeVisible(title);
    addAndMakeVisible(content.get());
    if(auto* tooltipClient = dynamic_cast<juce::SettableTooltipClient*>(content.get()))
    {
        tooltipClient->setTooltip(tooltip);
    }

    setTooltip(tooltip);
    setSize(200, 24);
}

void PropertyComponentBase::resized()
{
    auto& titleLookAndFeel = title.getLookAndFeel();
    auto const minimumHorizontalScale = title.getMinimumHorizontalScale();
    auto const font = titleLookAndFeel.getLabelFont(title).withHorizontalScale(minimumHorizontalScale);
    auto const bdSize = titleLookAndFeel.getLabelBorderSize(title);

    auto bounds = getLocalBounds();
    auto const textWidth = font.getStringWidth(title.getText());
    auto const textSize = textWidth + bdSize.getLeft() + bdSize.getRight();
    title.setBounds(bounds.removeFromLeft(textSize));
    if(content != nullptr)
    {
        content->setBounds(bounds);
    }
    title.setJustificationType(juce::Justification::centredLeft);
}

PropertyTextButton::PropertyTextButton(juce::String const& name, juce::String const& tooltip, std::function<void(void)> fn)
: PropertyComponent<juce::TextButton>(juce::translate(name), juce::translate(tooltip), nullptr)
{
    title.setVisible(false);
    entry.setButtonText(name);
    entry.setTooltip(tooltip);
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
: PropertyComponent<juce::Label>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRepaintsOnMouseActivity(true);
    entry.setEditable(true);
    entry.setTooltip(tooltip);
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
: PropertyComponent<NumberField>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    entry.setTooltip(juce::translate(tooltip));
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

PropertySlider::PropertySlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn)
: PropertyComponent<juce::Slider>(juce::translate(name), juce::translate(tooltip))
{
    entry.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    entry.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval));
    entry.setTooltip(juce::translate(tooltip));
    entry.setTextValueSuffix(suffix);
    entry.setScrollWheelEnabled(false);
    entry.onValueChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(entry.getValue()));
        }
    };
}

PropertyRangeSlider::PropertyRangeSlider(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float, float)> fn)
: PropertyComponent<juce::Slider>(juce::translate(name), juce::translate(tooltip))
{
    entry.setSliderStyle(juce::Slider::SliderStyle::TwoValueHorizontal);
    entry.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval));
    entry.setTooltip(juce::translate(tooltip));
    entry.setTextValueSuffix(suffix);
    entry.setScrollWheelEnabled(false);
    entry.onValueChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(entry.getMinValue()), static_cast<float>(entry.getMaxValue()));
        }
    };
}

PropertyToggle::PropertyToggle(juce::String const& name, juce::String const& tooltip, std::function<void(bool)> fn)
: PropertyComponent<juce::ToggleButton>(juce::translate(name), juce::translate(tooltip))
{
    entry.setTooltip(juce::translate(tooltip));
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
: PropertyComponent<juce::ComboBox>(juce::translate(name), juce::translate(tooltip))
{
    entry.setTooltip(tooltip);
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
: PropertyComponent<ColourButton>(juce::translate(name), juce::translate(tooltip))
{
    entry.setTitle(header);
    entry.setTooltip(juce::translate(tooltip));
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

ANALYSE_FILE_END
