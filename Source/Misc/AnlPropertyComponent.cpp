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

PropertyLabel::PropertyLabel(juce::String const& name, juce::String const& tooltip, juce::String const& text, callback_type fn)
: PropertyComponent<juce::Label>(name, tooltip, fn)
{
    entry.setRepaintsOnMouseActivity(true);
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
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())) - 1);
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

ANALYSE_FILE_END
