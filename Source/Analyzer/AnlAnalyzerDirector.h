#pragma once

#include "AnlAnalyzerModel.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Director
    : private juce::AsyncUpdater
    {
    public:
        Director(Accessor& accessor, PluginList::Scanner& pluginListScanner, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;
        
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
    private:
        
        void stopAnalysis(NotificationType const notification);
        void runAnalysis(NotificationType const notification);
        void updateZoomAccessors(NotificationType const notification);
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        
        Accessor& mAccessor;
        PluginList::Scanner& mPluginListScanner;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        std::atomic<ProcessState> mAnalysisState {ProcessState::available};
        std::future<std::tuple<std::vector<Plugin::Result>, NotificationType>> mAnalysisProcess;
        std::mutex mAnalysisMutex;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
