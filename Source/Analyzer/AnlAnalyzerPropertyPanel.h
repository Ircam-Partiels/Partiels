#pragma once

#include "AnlAnalyzerModel.h"
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
    private:
        
        
        class ColourSelector
        : public juce::ColourSelector
        , private juce::ChangeListener
        {
        public:
            ColourSelector();
            ~ColourSelector() override;
            
            std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;
        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Layout::PropertyLabel mAnalyzerName {juce::translate("Name"), juce::translate("The name of the analyzer")};
        Layout::PropertyLabel mPluginName {juce::translate("Plugin"), juce::translate("The name of the plugin")};
        Layout::PropertyComboBox mFeatures {juce::translate("Feature"), juce::translate("The active feature of the analyzer")};
        
        Layout::PropertyTitle mAnalysisParameters {juce::translate("Analysis Parameters"), juce::translate("The analysis parameters of the analyzer")};
        std::map<juce::String, std::unique_ptr<Layout::PropertyPanelBase>> mProperties;
        
        Layout::PropertyTitle mGraphicalParameters {juce::translate("Graphical Parameters"), juce::translate("The graphical parameters of the analyzer")};
        Layout::PropertyTextButton mColour {juce::translate("Color"), juce::translate("The current color")};
        Layout::PropertyComboBox mColourMap {juce::translate("Color Map"), juce::translate("The current color map")};
        
        Layout::PropertySection mPropertySection {"PARAMETERS", true};
    };
}

ANALYSE_FILE_END
