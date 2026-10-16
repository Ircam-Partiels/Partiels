#include "AnlPluginDescriptionPanel.h"

ANALYSE_FILE_BEGIN

Plugin::DescriptionPanel::DescriptionPanel()
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
    addChildComponent(downloadPluginButton);
    addChildComponent(mDownloadProgressBar);
    mDownloadProgressBar.setSize(0, PropertyComponentBase::defaultHeight / 3);
    mDownloadProgressBar.setPercentageDisplay(false);
    setSize(300, optimalHeight);
    setContent({}, {});
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
    setBounds(downloadPluginButton);
    setBounds(mDownloadProgressBar);
    mPropertyPluginDetails.setBounds(bounds);
}

void Plugin::DescriptionPanel::lookAndFeelChanged()
{
    auto const text = mPropertyPluginDetails.getText();
    mPropertyPluginDetails.clear();
    mPropertyPluginDetails.setText(text);
}

void Plugin::DescriptionPanel::setContent(Description const& description, WebReference const& webReference)
{
    mDescription = description;
    mWebReference = webReference;
    mIsDownloading = false;
    mDownloadProgress = -1.0;
    auto const isLocal = description.valid();

    mPropertyPluginName.setVisible(true);
    mPropertyPluginFeature.setVisible(isLocal);
    mPropertyPluginMaker.setVisible(true);
    mPropertyPluginVersion.setVisible(true);
    mPropertyPluginCategory.setVisible(isLocal);
    mPropertyPluginDetails.setVisible(true);

    mPropertyPluginName.entry.setText(isLocal ? description.name : webReference.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginFeature.entry.setText(description.output.name, juce::NotificationType::dontSendNotification);
    mPropertyPluginMaker.entry.setText(isLocal ? description.maker : webReference.maker, juce::NotificationType::dontSendNotification);
    mPropertyPluginVersion.entry.setText(juce::String(isLocal ? description.version : webReference.version), juce::NotificationType::dontSendNotification);
    mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, juce::NotificationType::dontSendNotification);
    juce::StringArray details{isLocal ? juce::StringArray{juce::String(description.output.description), description.details, description.copyright}
                                      : juce::StringArray{webReference.pluginDescription, webReference.libraryDescription}};
    details.removeEmptyStrings();
    mPropertyPluginDetails.setText(details.joinIntoString("\n"), juce::NotificationType::dontSendNotification);

    updateDownloadState();
    mPropertyPluginDetails.moveCaretToTop(false);
}

void Plugin::DescriptionPanel::setDownloadState(bool isDownloading, double progress)
{
    mDownloadProgress = progress;
    if(std::exchange(mIsDownloading, isDownloading) != isDownloading)
    {
        updateDownloadState();
    }
}

void Plugin::DescriptionPanel::updateDownloadState()
{
    // The internet-available plugin proposes a download, an update of the installed version if the local plugin is installed, or its website
    auto const isLocal = mDescription.valid();
    auto const isWeb = mWebReference.valid();
    downloadPluginButton.setVisible(isWeb && (mWebReference.getDownloadUrl().isNotEmpty() || mWebReference.websiteUrl.isNotEmpty()));
    if(mIsDownloading)
    {
        downloadPluginButton.entry.setButtonText(juce::translate("Downloading..."));
        downloadPluginButton.setTooltip(juce::translate("Click again to cancel the download"));
    }
    else if(mWebReference.getDownloadUrl().isNotEmpty())
    {
        downloadPluginButton.entry.setButtonText(juce::translate("Download"));
        if(isLocal && isWeb)
        {
            downloadPluginButton.setTooltip(juce::translate("Download the plugin package and update the installed version"));
        }
        else
        {
            downloadPluginButton.setTooltip(juce::translate("Download the plugin package and run its installer"));
        }
    }
    else
    {
        downloadPluginButton.setTooltip(juce::translate("Open the website of the plugin in the default browser"));
        downloadPluginButton.entry.setButtonText(juce::translate("Website"));
    }
    mDownloadProgressBar.setVisible(mIsDownloading);
    resized();
}

ANALYSE_FILE_END
