#pragma once

#include "AnlPluginModel.h"

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

        void setDescription(Description const& description);
        void setWebReference(WebReference const& webReference);

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;

    private:
        PropertyLabel mPropertyPluginName{juce::translate("Name"), juce::translate("The name of the plugin")};
        PropertyLabel mPropertyPluginFeature{juce::translate("Feature"), juce::translate("The feature of the plugin")};
        PropertyLabel mPropertyPluginMaker{juce::translate("Maker"), juce::translate("The maker of the plugin")};
        PropertyLabel mPropertyPluginVersion{juce::translate("Version"), juce::translate("The version of the plugin")};
        PropertyLabel mPropertyPluginCategory{juce::translate("Category"), juce::translate("The category of the plugin")};
        PropertyTextButton mPropertyPluginDownload;
        juce::TextEditor mPropertyPluginDetails;
        juce::String mPluginDownloadUrl;
    };
} // namespace Plugin

ANALYSE_FILE_END
