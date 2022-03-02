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

        PropertyLabel mPropertyPluginName{"Name", "The name of the plugin"};
        PropertyLabel mPropertyPluginFeature{"Feature", "The feature of the plugin"};
        PropertyLabel mPropertyPluginMaker{"Maker", "The maker of the plugin"};
        PropertyLabel mPropertyPluginVersion{"Version", "The version of the plugin"};
        PropertyLabel mPropertyPluginCategory{"Category", "The category of the plugin"};
        juce::TextEditor mPropertyPluginDetails;
    };
} // namespace Track

ANALYSE_FILE_END
