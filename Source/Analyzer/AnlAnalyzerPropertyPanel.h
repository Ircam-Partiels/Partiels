#pragma once

#include "AnlAnalyzerModel.h"
#include "AnlAnalyzerProcessor.h"
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
        
        void updateProcessorProperties();
        void updateGraphicalProperties();
        void updatePluginProperties();
        
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
        
        Layout::PropertyLabel mNameProperty {juce::translate("Name"), juce::translate("The name of the analyzer")};
        
        Layout::PropertySection mProcessorSection {juce::translate("PROCESSOR"), true,
            juce::translate("The processor parameters of the analyzer")};
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mProcessorProperties;
        
        Layout::PropertySection mGraphicalSection {juce::translate("GRAPHICAL"), true,
            juce::translate("The graphical parameters of the analyzer")};
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mGraphicalProperties;
        
        Layout::PropertySection mPluginSection {juce::translate("PLUGIN"), true,
            juce::translate("The plugin information")};
        std::vector<std::unique_ptr<Layout::PropertyPanelBase>> mPluginProperties;
        
        Layout::PropertyTextButton mColour {juce::translate("Color"), juce::translate("The current color")};
        Layout::PropertyComboBox mColourMap {juce::translate("Color Map"), juce::translate("The current color map")};
    };
}

ANALYSE_FILE_END
