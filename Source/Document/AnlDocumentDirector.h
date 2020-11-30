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
        ~Director() override;
        
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
        
        struct results_container
        {
            results_container(double v1, Zoom::Range v2, std::vector<Analyzer::Result>&& v3)
            : minimumLength(v1), range(v2), results(std::forward<std::vector<Analyzer::Result>>(v3))
            {
                
            }
            
            double minimumLength = 0.0;
            Zoom::Range range {};
            std::vector<Analyzer::Result> results {};
        };
        
        std::mutex mMutex;
        std::vector<std::tuple<std::thread, std::reference_wrapper<Analyzer::Accessor>, std::shared_ptr<results_container>, NotificationType>> mProcesses;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
