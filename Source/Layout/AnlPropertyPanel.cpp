#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

Layout::PropertyPanelBase::PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& name, juce::String const& tooltip, Positioning p)
: positioning(p)
, content(std::move(c))
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
    auto bounds = getLocalBounds();
    auto const isLeft = positioning == Positioning::left;
    auto const font = title.getLookAndFeel().getLabelFont(title).withHorizontalScale(title.getMinimumHorizontalScale());
    auto const borderSize = title.getLookAndFeel().getLabelBorderSize(title);
    auto const textSize = isLeft ? font.getStringWidthFloat(title.getText()) + borderSize.getLeft() + borderSize.getRight() : font.getHeight() + borderSize.getTop() + borderSize.getBottom();
    title.setBounds(isLeft ? bounds.removeFromLeft(static_cast<int>(std::ceil(textSize))) : bounds.removeFromTop(static_cast<int>(std::ceil(textSize))));
    if(content != nullptr)
    {
        content->setBounds(bounds);
    }
    title.setJustificationType(isLeft ? juce::Justification::centredLeft : juce::Justification::centredTop);
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
    entry.onTextChange = [&]()
    {
        if(callback != nullptr)
        {
            callback(entry);
        }
    };
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

ANALYSE_FILE_END
