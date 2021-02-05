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
        
        void updateProcessorProperties();
        void updateGraphicalProperties();
        void updatePluginProperties();
        
        using PropertyLabel = Layout::PropertyLabel;
        
        class PropertyInt
        : public Layout::PropertyPanel<NumberField>
        {
        public:
            PropertyInt(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<int> const& range, std::function<void(int)> fn);
            ~PropertyInt() override = default;
        };
        
        class PropertyFloat
        : public Layout::PropertyPanel<NumberField>
        {
        public:
            PropertyFloat(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, std::function<void(float)> fn, size_t numDecimals = 2);
            ~PropertyFloat() override = default;
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
        
        PropertyLabel mPropertyName {juce::translate("Name"), juce::translate("The name of the analyzer")};
        
        PropertyList mPropertyWindowType;
        PropertyList mPropertyWindowSize;
        PropertyList mPropertyWindowOverlapping;
        PropertyInt mPropertyBlockSize;
        PropertyInt mPropertyStepSize;
        
        ConcertinaPanel mProcessorSection {juce::translate("PROCESSOR"), true,
            juce::translate("The processor parameters of the analyzer")};
        ConcertinaPanel mGraphicalSection {juce::translate("GRAPHICAL"), true,
            juce::translate("The graphical parameters of the analyzer")};
        ConcertinaPanel mPluginSection {juce::translate("PLUGIN"), true,
            juce::translate("The plugin information")};
        
        std::map<std::string, std::unique_ptr<Layout::PropertyPanelBase>> mParameterProperties;
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mGraphicalProperties;
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mPluginProperties;
        
        Layout::PropertyTextButton mColour {juce::translate("Color"), juce::translate("The current color")};
        Layout::PropertyComboBox mColourMap {juce::translate("Color Map"), juce::translate("The current color map")};
        
        
        juce::Viewport mViewport;
        juce::ComponentBoundsConstrainer mBoundsConstrainer;
        FloatingWindow mFloatingWindow {"Properties"};
    };
}

ANALYSE_FILE_END
