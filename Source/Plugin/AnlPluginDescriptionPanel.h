#pragma once

#include "AnlPluginListWebDownloader.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class DescriptionPanel
    : public juce::Component
    {
    public:
        static auto constexpr optimalHeight = PropertyComponentBase::defaultHeight * 7;

        DescriptionPanel();
        ~DescriptionPanel() override = default;

        //! @brief Set the content of the panel
        //! @param description The description of the local plugin (invalid if the plugin is not installed)
        //! @param webReference The internet-available plugin (invalid if the plugin is not available on the internet)
        void setContent(Description const& description, WebReference const& webReference);

        //! @brief The internet-available plugin of the content (invalid if the plugin is not available on the internet)
        WebReference const& getWebReference() const noexcept
        {
            return mWebReference;
        }

        //! @brief Set the download state of the internet-available plugin of the content
        //! @details The state is reset when the content changes
        //! @param isDownloading True if the package of the plugin is being downloaded
        //! @param progress The download progress of the package (between 0 and 1, negative if the progress is unknown)
        void setDownloadState(bool isDownloading, double progress);

        //! @brief The button that downloads the plugin, the click behavior is defined by the owner of the panel
        PropertyTextButton downloadPluginButton{juce::translate("Website"), juce::translate("Open the website of the plugin in the default browser"), nullptr};

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;

    private:
        void updateDownloadState();

        PropertyLabel mPropertyPluginName{juce::translate("Name"), juce::translate("The name of the plugin")};
        PropertyLabel mPropertyPluginFeature{juce::translate("Feature"), juce::translate("The feature of the plugin")};
        PropertyLabel mPropertyPluginMaker{juce::translate("Maker"), juce::translate("The maker of the plugin")};
        PropertyLabel mPropertyPluginVersion{juce::translate("Version"), juce::translate("The version of the plugin")};
        PropertyLabel mPropertyPluginCategory{juce::translate("Category"), juce::translate("The category of the plugin")};
        juce::TextEditor mPropertyPluginDetails;
        Description mDescription{};
        WebReference mWebReference{};
        bool mIsDownloading{false};
        double mDownloadProgress{-1.0};
        juce::ProgressBar mDownloadProgressBar{mDownloadProgress};
    };
} // namespace Plugin

ANALYSE_FILE_END
