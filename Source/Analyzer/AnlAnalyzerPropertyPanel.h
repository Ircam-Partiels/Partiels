#pragma once

#include "AnlAnalyzerModel.h"
#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
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
        
        class PropertyTextButton
        : public PropertyComponent<juce::TextButton>
        {
        public:
            PropertyTextButton(juce::String const& name, juce::String const& tooltip, std::function<void(void)> fn);
            ~PropertyTextButton() override = default;
            
            // juce::Component
            void resized() override;
        };
        
        class PropertyText
        : public PropertyComponent<juce::Label>
        {
        public:
            PropertyText(juce::String const& name, juce::String const& tooltip, std::function<void(juce::String)> fn);
            ~PropertyText() override = default;
        };
        
        class PropertyLabel
        : public PropertyText
        {
        public:
            PropertyLabel(juce::String const& name, juce::String const& tooltip);
            ~PropertyLabel() override = default;
        };
        
        class PropertyNumber
        : public PropertyComponent<NumberField>
        {
        public:
            PropertyNumber(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn);
            ~PropertyNumber() override = default;
        };

        class PropertyList
        : public PropertyComponent<juce::ComboBox>
        {
        public:
            PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn);
            ~PropertyList() override = default;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        
        PropertyText mPropertyName;
        
        PropertyList mPropertyWindowType;
        PropertyList mPropertyWindowSize;
        PropertyList mPropertyWindowOverlapping;
        PropertyNumber mPropertyBlockSize;
        PropertyNumber mPropertyStepSize;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;
        PropertyTextButton mPropertyResetProcessor;
        
        PropertyTextButton mPropertyColourSelector;
        PropertyList mPropertyColourMap;
        PropertyNumber mPropertyValueRangeMin;
        PropertyNumber mPropertyValueRangeMax;
        
        PropertyLabel mPropertyPluginName {"Name", "The name of the plugin"};
        PropertyLabel mPropertyPluginFeature {"Feature", "The feature of the plugin"};
        PropertyLabel mPropertyPluginMaker {"Maker", "The maker of the plugin"};
        PropertyLabel mPropertyPluginVersion {"Version", "The version of the plugin"};
        PropertyLabel mPropertyPluginCategory {"Category", "The category of the plugin"};
        juce::TextEditor mPropertyPluginDetails;
        
        ConcertinaTable mProcessorSection {juce::translate("PROCESSOR"), true,
            juce::translate("The processor parameters of the analyzer")};
        ConcertinaTable mGraphicalSection {juce::translate("GRAPHICAL"), true,
            juce::translate("The graphical parameters of the analyzer")};
        ConcertinaTable mPluginSection {juce::translate("PLUGIN"), true,
            juce::translate("The plugin information")};
        
        juce::Viewport mViewport;
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        FloatingWindow mFloatingWindow {"Properties"};
        static auto constexpr sInnerWidth = 300;
    };
}

ANALYSE_FILE_END
