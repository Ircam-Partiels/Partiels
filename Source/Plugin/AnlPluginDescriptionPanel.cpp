#include "AnlPluginDescriptionPanel.h"

ANALYSE_FILE_BEGIN

Plugin::DescriptionPanel::DescriptionPanel()
: mPropertyPluginDownload(juce::translate("Download"), juce::translate("Proceed to plugin download page"), [this]()
                          {
                              if(mPluginDownloadUrl.isNotEmpty())
                              {
                                  juce::URL(mPluginDownloadUrl).launchInDefaultBrowser();
                              }
                          })
{
    mPropertyPluginDetails.setTooltip(juce::translate("The details of the plugin"));
    mPropertyPluginDetails.setJustification(juce::Justification::topLeft);
    mPropertyPluginDetails.setMultiLine(true);
    mPropertyPluginDetails.setReadOnly(true);
    mPropertyPluginDetails.setScrollbarsShown(true);

    addAndMakeVisible(mPropertyPluginName);
    addAndMakeVisible(mPropertyPluginFeature);
    addAndMakeVisible(mPropertyPluginMaker);
    addAndMakeVisible(mPropertyPluginVersion);
    addAndMakeVisible(mPropertyPluginCategory);
    addAndMakeVisible(mPropertyPluginDetails);
    addChildComponent(mPropertyPluginDownload);
    setSize(300, optimalHeight);
    setDescription({});
}

void Plugin::DescriptionPanel::resized()
{
    auto bounds = getLocalBounds();
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            auto const height = component.getHeight();
            component.setBounds(bounds.removeFromTop(height).withHeight(height));
        }
    };
    setBounds(mPropertyPluginName);
    setBounds(mPropertyPluginFeature);
    setBounds(mPropertyPluginMaker);
    setBounds(mPropertyPluginVersion);
    setBounds(mPropertyPluginCategory);
    setBounds(mPropertyPluginDownload);
    mPropertyPluginDetails.setBounds(bounds);
}

void Plugin::DescriptionPanel::lookAndFeelChanged()
{
    auto const text = mPropertyPluginDetails.getText();
    mPropertyPluginDetails.clear();
    mPropertyPluginDetails.setText(text);
}

void Plugin::DescriptionPanel::setDescription(Description const& description)
{
    mPropertyPluginName.setVisible(true);
    mPropertyPluginFeature.setVisible(true);
    mPropertyPluginMaker.setVisible(true);
    mPropertyPluginVersion.setVisible(true);
    mPropertyPluginCategory.setVisible(true);
    mPropertyPluginDownload.setVisible(false);
    mPropertyPluginDetails.setVisible(true);

    mPropertyPluginName.entry.setText(description.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginFeature.entry.setText(description.output.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginMaker.entry.setText(description.maker, juce::NotificationType::dontSendNotification);
    mPropertyPluginVersion.entry.setText(juce::String(description.version), juce::NotificationType::dontSendNotification);
    mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, juce::NotificationType::dontSendNotification);
    juce::StringArray details{juce::String(description.output.description), description.details};
    details.removeEmptyStrings();
    mPropertyPluginDetails.setText(details.joinIntoString("\n"), juce::NotificationType::dontSendNotification);
    resized();
    mPropertyPluginDetails.moveCaretToTop(false);
}

void Plugin::DescriptionPanel::setWebReference(WebReference const& webReference)
{
    mPropertyPluginName.setVisible(true);
    mPropertyPluginFeature.setVisible(false);
    mPropertyPluginMaker.setVisible(true);
    mPropertyPluginVersion.setVisible(false);
    mPropertyPluginCategory.setVisible(false);
    mPropertyPluginDownload.setVisible(true);
    mPropertyPluginDetails.setVisible(true);

    mPropertyPluginName.entry.setText(webReference.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginMaker.entry.setText(webReference.maker, juce::NotificationType::dontSendNotification);
    mPluginDownloadUrl = webReference.downloadUrl;
    mPropertyPluginDownload.setEnabled(mPluginDownloadUrl.isNotEmpty());
    juce::StringArray details{webReference.pluginDescription, webReference.libraryDescription};
    details.removeEmptyStrings();
    mPropertyPluginDetails.setText(details.joinIntoString("\n"), juce::NotificationType::dontSendNotification);
    resized();
    mPropertyPluginDetails.moveCaretToTop(false);
}

ANALYSE_FILE_END
