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
        ControlPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~ControlPanel() override;
        
        void resized() override;
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        PluginList::Table mPluginListTable;
        
        struct Section
        {
            Section(Analyzer::Accessor& acsr)
            : accessor(acsr)
            {
            }
            Analyzer::Accessor& accessor;
            Analyzer::Thumbnail thumbnail {accessor};
            Tools::ColouredPanel separator;
        };
        
        std::vector<std::unique_ptr<Section>> mSections;
        juce::TextButton mAddButton {juce::translate("+")};
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
    };
}

ANALYSE_FILE_END
