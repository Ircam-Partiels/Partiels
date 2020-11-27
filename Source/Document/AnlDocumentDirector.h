#pragma once

#include "AnlDocumentModel.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    : private juce::AsyncUpdater
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~Director();
        
        void addAnalysis(AlertType alertType);
        
    private:
        
        void setupDocument(Document::Accessor& acsr);
        void setupAnalyzer(Analyzer::Accessor& acsr);
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        std::vector<std::thread> mThreads;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
