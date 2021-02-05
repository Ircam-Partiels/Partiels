#pragma once

#include "AnlAnalyzerModel.h"
#include "../Misc/AnlMisc.h"
#include "../Layout/AnlLayout.h"

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
        
        class PropertyText
        : public Layout::PropertyPanel<juce::Label>
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
        : public Layout::PropertyPanel<NumberField>
        {
        public:
            PropertyNumber(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn);
            ~PropertyNumber() override = default;
        };

        class PropertyList
        : public Layout::PropertyPanel<juce::ComboBox>
        {
        public:
            PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn);
            ~PropertyList() override = default;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        PropertyText mPropertyName;
        
        PropertyList mPropertyWindowType;
        PropertyList mPropertyWindowSize;
        PropertyList mPropertyWindowOverlapping;
        PropertyNumber mPropertyBlockSize;
        PropertyNumber mPropertyStepSize;
        std::map<std::string, std::unique_ptr<Layout::PropertyPanelBase>> mParameterProperties;
        
        juce::TextButton mPropertyColourSelector {juce::translate("Color"), juce::translate("The current color")};
        PropertyList mPropertyColourMap;
        
        PropertyLabel mPropertyPluginName {"Name", "The name of the plugin"};
        PropertyLabel mPropertyPluginFeature {"Feature", "The feature of the plugin"};
        PropertyLabel mPropertyPluginMaker {"Maker", "The maker of the plugin"};
        PropertyLabel mPropertyPluginVersion {"Version", "The version of the plugin"};
        PropertyLabel mPropertyPluginCategory {"Category", "The category of the plugin"};
        juce::TextEditor mPropertyPluginDetails;
        
        ConcertinaPanel mProcessorSection {juce::translate("PROCESSOR"), true,
            juce::translate("The processor parameters of the analyzer")};
        ConcertinaPanel mGraphicalSection {juce::translate("GRAPHICAL"), true,
            juce::translate("The graphical parameters of the analyzer")};
        ConcertinaPanel mPluginSection {juce::translate("PLUGIN"), true,
            juce::translate("The plugin information")};
        
        juce::Viewport mViewport;
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        FloatingWindow mFloatingWindow {"Properties"};
    };
}

ANALYSE_FILE_END
