#pragma once

#include "AnlTrackModel.h"
#include "AnlTrackProgressBar.h"
#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyPanel
    : public juce::Component
    {
    public:
        PropertyPanel(Accessor& accessor);
        ~PropertyPanel() override;
        
        // juce::Component
        void resized() override;
        
        void show();
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        BoundsListener mBoundsListener;
        
        PropertyText mPropertyName;
        
        PropertyList mPropertyWindowType;
        PropertyList mPropertyWindowSize;
        PropertyList mPropertyWindowOverlapping;
        PropertyNumber mPropertyBlockSize;
        PropertyNumber mPropertyStepSize;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;
        PropertyList mPropertyPreset;
        ProgressBar mProgressBar {mAccessor};
        
        PropertyList mPropertyColourMap;
        PropertySlider mPropertyColourMapAlpha;
        PropertyTextButton mPropertyForegroundColour;
        PropertyTextButton mPropertyBackgroundColour;
        PropertyList mPropertyValueRangeMode;
        PropertyNumber mPropertyValueRangeMin;
        PropertyNumber mPropertyValueRangeMax;
        PropertyRangeSlider mPropertyValueRange;
        PropertyNumber mPropertyNumBins;
        
        PropertyLabel mPropertyPluginName {"Name", "The name of the plugin"};
        PropertyLabel mPropertyPluginFeature {"Feature", "The feature of the plugin"};
        PropertyLabel mPropertyPluginMaker {"Maker", "The maker of the plugin"};
        PropertyLabel mPropertyPluginVersion {"Version", "The version of the plugin"};
        PropertyLabel mPropertyPluginCategory {"Category", "The category of the plugin"};
        juce::TextEditor mPropertyPluginDetails;
        
        ConcertinaTable mProcessorSection {juce::translate("PROCESSOR"), true,
            juce::translate("The processor parameters of the track")};
        ConcertinaTable mGraphicalSection {juce::translate("GRAPHICAL"), true,
            juce::translate("The graphical parameters of the track")};
        ConcertinaTable mPluginSection {juce::translate("PLUGIN"), true,
            juce::translate("The plugin information")};
        
        juce::Viewport mViewport;
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        FloatingWindow mFloatingWindow {"Properties"};
        static auto constexpr sInnerWidth = 300;
    };
}

ANALYSE_FILE_END
