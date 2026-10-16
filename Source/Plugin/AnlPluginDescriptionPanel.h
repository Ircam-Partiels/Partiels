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

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;

    private:
        //! @brief Update the text and the state of the download button
        void updateButton();

        PropertyLabel mPropertyPluginName{juce::translate("Name"), juce::translate("The name of the plugin")};
        PropertyLabel mPropertyPluginFeature{juce::translate("Feature"), juce::translate("The feature of the plugin")};
        PropertyLabel mPropertyPluginMaker{juce::translate("Maker"), juce::translate("The maker of the plugin")};
        PropertyLabel mPropertyPluginVersion{juce::translate("Version"), juce::translate("The version of the plugin")};
        PropertyLabel mPropertyPluginCategory{juce::translate("Category"), juce::translate("The category of the plugin")};
        PropertyTextButton mPropertyPluginDownload;
        juce::TextEditor mPropertyPluginDetails;
        Description mDescription{};
        WebReference mWebReference{};
        PluginList::WebDownloader mWebDownloader;
        bool mDownloading{false};
    };
} // namespace Plugin

ANALYSE_FILE_END
