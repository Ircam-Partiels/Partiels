#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlColouredPanel.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Analyzer/AnlAnalyzerThumbnail.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ControlPanel
    : public juce::Component
    {
    public:
        ControlPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager& audioFormatManager);
        ~ControlPanel() override;
        
        void resized() override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor::Listener mListener;
        PluginList::Table mPluginListTable;
        struct Section
        {
            Section(Analyzer::Accessor& acsr)
            : thumbnail(acsr)
            , processor(acsr)
            {
                
            }
            Analyzer::Thumbnail thumbnail;
            Analyzer::Processor processor;
            Tools::ColouredPanel separator;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
    };
}

ANALYSE_FILE_END
