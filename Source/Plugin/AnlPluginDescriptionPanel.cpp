#include "AnlPluginDescriptionPanel.h"

ANALYSE_FILE_BEGIN

Plugin::DescriptionPanel::DescriptionPanel()
: mPropertyPluginDownload(juce::translate("Website"), juce::translate("The website of the plugin"), [this]()
                          {
                              if(mWebReference.getDownloadUrl().isNotEmpty())
                              {
                                  if(mDownloading)
                                  {
                                      return;
                                  }
                                  mDownloading = true;
                                  updateButton();
                                  mWebDownloader.download(mWebReference, [this](juce::Result result, juce::File installer)
                                                          {
                                                              mDownloading = false;
                                                              updateButton();
                                                              if(!result.wasOk())
                                                              {
                                                                  auto const options = juce::MessageBoxOptions()
                                                                                           .withIconType(juce::AlertWindow::WarningIcon)
                                                                                           .withTitle(juce::translate("Plugin installation failed!"))
                                                                                           .withMessage(juce::translate(result.getErrorMessage()))
                                                                                           .withButton(juce::translate("Ok"));
                                                                  juce::AlertWindow::showAsync(options, nullptr);
                                                                  return;
                                                              }
#if JUCE_MAC || JUCE_WINDOWS
                                                              // The pkg installer is opened with the system installer on MacOS, the exe installer is executed on Windows
                                                              if(!installer.existsAsFile() || !installer.startAsProcess())
#else

                                                              // The file or the directory is revealed in the file manager of the system
                                                              if(installer.existsAsFile())
                                                              {
                                                                  installer.revealToUser();
                                                              }
                                                              else
#endif
                                                              {
                                                                  auto const options = juce::MessageBoxOptions()
                                                                                           .withIconType(juce::AlertWindow::WarningIcon)
                                                                                           .withTitle(juce::translate("Plugin installation failed!"))
                                                                                           .withMessage(juce::translate("The plugin installer could not be launched."))
                                                                                           .withButton(juce::translate("Ok"));
                                                                  juce::AlertWindow::showAsync(options, nullptr);
                                                              }
                                                          });
                              }
                              else if(mWebReference.websiteUrl.isNotEmpty())
                              {
                                  juce::URL(mWebReference.websiteUrl).launchInDefaultBrowser();
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
    setBounds(mPropertyPluginDownload);
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
    auto const isLocal = description.valid();
    auto const isWeb = webReference.valid();

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

    // The internet-available plugin proposes an installation, an update if the local plugin is installed, or its website
    mPropertyPluginDownload.setVisible(isWeb && (webReference.getDownloadUrl().isNotEmpty() || webReference.websiteUrl.isNotEmpty()));
    updateButton();
    resized();
    mPropertyPluginDetails.moveCaretToTop(false);
}

void Plugin::DescriptionPanel::updateButton()
{
    mPropertyPluginDownload.setEnabled(!mDownloading);
    if(mWebReference.getDownloadUrl().isNotEmpty())
    {
        auto const updateAvailable = mDescription.valid() && mWebReference.valid();
        if(updateAvailable)
        {
            mPropertyPluginDownload.setTooltip(juce::translate("Download and update the plugin"));
        }
        else
        {
            mPropertyPluginDownload.setTooltip(juce::translate("Download and install the plugin"));
        }
        if(mDownloading)
        {
            mPropertyPluginDownload.entry.setButtonText(juce::translate("Downloading..."));
        }
        else if(updateAvailable)
        {
            mPropertyPluginDownload.entry.setButtonText(juce::translate("Update"));
        }
        else
        {
            mPropertyPluginDownload.entry.setButtonText(juce::translate("Install"));
        }
    }
    else
    {
        mPropertyPluginDownload.setTooltip(juce::translate("The website of the plugin"));
        mPropertyPluginDownload.entry.setButtonText(juce::translate("Website"));
    }
}

ANALYSE_FILE_END
