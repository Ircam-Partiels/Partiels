#include "AnlPluginDescriptionPanel.h"

ANALYSE_FILE_BEGIN

Plugin::DescriptionPanel::DescriptionPanel()
{
    mPropertyPluginDetails.setTooltip(juce::translate("The details of the plugin"));
    mPropertyPluginDetails.setSize(300, 48);
    mPropertyPluginDetails.setJustification(juce::Justification::horizontallyJustified);
    mPropertyPluginDetails.setMultiLine(true);
    mPropertyPluginDetails.setReadOnly(true);
    mPropertyPluginDetails.setScrollbarsShown(true);

    addAndMakeVisible(mPropertyPluginName);
    addAndMakeVisible(mPropertyPluginFeature);
    addAndMakeVisible(mPropertyPluginMaker);
    addAndMakeVisible(mPropertyPluginVersion);
    addAndMakeVisible(mPropertyPluginCategory);
    addAndMakeVisible(mPropertyPluginDetails);
    setSize(300, optimalHeight);
}

void Plugin::DescriptionPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyPluginName);
    setBounds(mPropertyPluginFeature);
    setBounds(mPropertyPluginMaker);
    setBounds(mPropertyPluginVersion);
    setBounds(mPropertyPluginCategory);
    setBounds(mPropertyPluginDetails);
}

void Plugin::DescriptionPanel::lookAndFeelChanged()
{
    auto const text = mPropertyPluginDetails.getText();
    mPropertyPluginDetails.clear();
    mPropertyPluginDetails.setText(text);
}

void Plugin::DescriptionPanel::setDescription(Description const& description)
{
    mPropertyPluginName.entry.setText(description.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginFeature.entry.setText(description.output.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginMaker.entry.setText(description.maker, juce::NotificationType::dontSendNotification);
    mPropertyPluginVersion.entry.setText(juce::String(description.version), juce::NotificationType::dontSendNotification);
    mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, juce::NotificationType::dontSendNotification);
    auto const details = juce::String(description.output.description) + (description.output.description.empty() ? "" : "\n") + description.details;
    mPropertyPluginDetails.setText(details, juce::NotificationType::dontSendNotification);
    resized();
}

ANALYSE_FILE_END
