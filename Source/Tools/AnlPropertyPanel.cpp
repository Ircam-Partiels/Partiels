#include "AnlPropertyPanel.h"

ANALYSE_FILE_BEGIN

Tools::PropertyPanelBase::PropertyPanelBase(std::unique_ptr<juce::Component> c, juce::String const& text, juce::String const& tooltip, Positioning p)
: positioning(p)
, content(std::move(c))
{
    title.setText(text + ":", juce::NotificationType::dontSendNotification);
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

void Tools::PropertyPanelBase::resized()
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

ANALYSE_FILE_END
