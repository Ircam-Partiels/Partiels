#pragma once

#include "AnlAnalyzerModel.h"
#include "../Layout/AnlLayout.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class PropertyPanel
    : public juce::Component
    , private juce::ChangeListener
    {
    public:
        
        PropertyPanel(Accessor& accessor);
        ~PropertyPanel() override;
        
        // juce::Component
        void resized() override;
        
        std::function<void(void)> onAnalyse = nullptr;
    private:
        
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Layout::PropertyLabel mPluginName {juce::translate("Plugin"), juce::translate("The name of the analyzer")};
        Layout::PropertyComboBox mFeatures {juce::translate("Feature"), juce::translate("The active feature of the analyzer")};
        Layout::PropertyTitle mAnalysisParameters {juce::translate("Analysis Parameters"), juce::translate("The analysis parameters of the pluganalyzerin")};
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mProperties;
        Layout::PropertyTitle mGraphicalParameters {juce::translate("Graphical Parameters"), juce::translate("The graphical parameters of the analyzer")};
        Layout::PropertyTextButton mColour {juce::translate("Color"), juce::translate("The current color")};
        Layout::PropertyComboBox mColourMap {juce::translate("Color Map"), juce::translate("The current color map")};
        Tools::ColouredPanel mBottomSeparator;
        Layout::PropertyTextButton mAnalyse {juce::translate("Analyse"), juce::translate("Run the analysis")};
        
        Layout::PropertySection mPropertySection;
    
        juce::ColourSelector mColourSelector;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyPanel)
    };
}

ANALYSE_FILE_END
