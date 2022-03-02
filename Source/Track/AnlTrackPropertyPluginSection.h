#pragma once

#include "AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyPluginSection
    : public juce::Component
    {
    public:
        PropertyPluginSection(Director& director);
        ~PropertyPluginSection() override;

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        PropertyLabel mPropertyPluginName{juce::translate("Name"), juce::translate("The name of the plugin")};
        PropertyLabel mPropertyPluginFeature{juce::translate("Feature"), juce::translate("The feature of the plugin")};
        PropertyLabel mPropertyPluginMaker{juce::translate("Maker"), juce::translate("The maker of the plugin")};
        PropertyLabel mPropertyPluginVersion{juce::translate("Version"), juce::translate("The version of the plugin")};
        PropertyLabel mPropertyPluginCategory{juce::translate("Category"), juce::translate("The category of the plugin")};
        juce::TextEditor mPropertyPluginDetails;
    };
} // namespace Track

ANALYSE_FILE_END
