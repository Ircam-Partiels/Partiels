#pragma once

#include "AnlDocumentModel.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~Director();
        
        void addAnalysis(AlertType alertType);
        
    private:
        
        void setupDocument(Document::Accessor& acsr);
        void setupAnalyzer(Analyzer::Accessor& acsr);
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
