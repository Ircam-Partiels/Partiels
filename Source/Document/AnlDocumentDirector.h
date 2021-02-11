#pragma once

#include "AnlDocumentModel.h"
#include "../Analyzer/AnlAnalyzerDirector.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager);
        ~Director();
        
        void addAnalysis(AlertType const alertType, NotificationType const notification);
        void removeAnalysis(juce::String const identifier, NotificationType const notification);
        
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        std::vector<std::unique_ptr<Analyzer::Director>> mAnalyzers;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
