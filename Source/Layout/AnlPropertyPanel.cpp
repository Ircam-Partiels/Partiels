#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

Layout::PropertyPanelBase::PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip)
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

void Layout::PropertyPanelBase::resized()
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

Layout::PropertyTitle::PropertyTitle(juce::String const& name, juce::String const& tooltip)
: Layout::PropertyPanel<juce::Component>(name, tooltip, nullptr)
{
    title.setFont(title.getFont().boldened());
}

Layout::PropertyTextButton::PropertyTextButton(juce::String const& name, juce::String const& tooltip, callback_type fn)
: Layout::PropertyPanel<juce::TextButton>(name, tooltip, fn)
{
    title.setVisible(false);
    entry.setButtonText(name);
    entry.setTooltip(tooltip);
    entry.onClick = [&]()
    {
        if(callback != nullptr)
        {
            callback(entry);
        }
    };
}

void Layout::PropertyTextButton::resized()
{
    entry.setBounds(getLocalBounds());
}

Layout::PropertyLabel::PropertyLabel(juce::String const& name, juce::String const& tooltip, juce::String const& text, callback_type fn)
: Layout::PropertyPanel<juce::Label>(name, tooltip, fn)
{
    entry.setEditable(true);
    entry.setTooltip(tooltip);
    entry.setText(text, juce::NotificationType::dontSendNotification);
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
    entry.setBorderSize({});
    entry.onEditorShow = [&]()
    {
        if(auto* editor = entry.getCurrentTextEditor())
        {
            auto const font = entry.getFont();
            editor->setFont(font);
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())));
            editor->setBorder(entry.getBorderSize());
            editor->setJustification(entry.getJustificationType());
        }
    };
    entry.onTextChange = [&]()
    {
        if(callback != nullptr)
        {
            callback(entry);
        }
    };
}

Layout::PropertyText::PropertyText(juce::String const& name, juce::String const& tooltip, juce::String const& text, callback_type fn)
: Layout::PropertyLabel(name, tooltip, text, fn)
{
    entry.setJustificationType(juce::Justification::centredLeft);
    setSize(200, 48);
}

void Layout::PropertyText::PropertyText::resized()
{
    auto& titleLookAndFeel = title.getLookAndFeel();
    auto const font = titleLookAndFeel.getLabelFont(title);
    auto const bdSize = titleLookAndFeel.getLabelBorderSize(title);
    
    auto bounds = getLocalBounds();
    auto const textHeight = static_cast<int>(std::ceil(font.getHeight())) + bdSize.getTop() + bdSize.getBottom();
    title.setBounds(bounds.removeFromTop(textHeight));
    title.setJustificationType(juce::Justification::centredLeft);
    entry.setBounds(bounds);
}

Layout::PropertyComboBox::PropertyComboBox(juce::String const& name, juce::String const& tooltip, juce::StringArray const& items, size_t index, callback_type fn)
: Layout::PropertyPanel<juce::ComboBox>(name, tooltip, fn)
{
    entry.setTooltip(tooltip);
    entry.addItemList(items, 1);
    entry.setSelectedItemIndex(static_cast<int>(index), juce::NotificationType::dontSendNotification);
    entry.setJustificationType(juce::Justification::centredRight);
    entry.onChange = [&]()
    {
        if(callback != nullptr)
        {
            callback(entry);
        }
    };
}

Layout::PropertySlider::PropertySlider(juce::String const& name, juce::String const& tooltip, juce::Range<double> const& range, double interval, juce::String const& textValueSuffix, double value, callback_type fn)
: Layout::PropertyPanel<juce::Slider>(name, tooltip, fn)
{
    entry.setTooltip(tooltip);
    entry.setRange(range, interval);
    entry.setValue(value, juce::NotificationType::dontSendNotification);
    entry.setTextValueSuffix(textValueSuffix),
    entry.onValueChange = [&]()
    {
        if(callback != nullptr)
        {
            callback(entry);
        }
    };
}

ANALYSE_FILE_END
