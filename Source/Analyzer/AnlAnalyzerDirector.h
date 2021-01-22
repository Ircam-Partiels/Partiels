#pragma once

#include "AnlAnalyzerModel.h"
#include "AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Director
    : private juce::AsyncUpdater
    {
    public:
        Director(Accessor& accessor);
        ~Director() override;
        
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
        
    private:
        
        void updateProcessor(NotificationType const notification);
        void sanitizeProcessor(NotificationType const notification);
        void runAnalysis(NotificationType const notification);
        void runRendering(NotificationType const notification);
        void updateFromResults(NotificationType const notification);
        void updateZoomRange(NotificationType const notification);
        
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
        std::atomic<ProcessState> mAnalysisState {ProcessState::available};
        std::future<std::tuple<std::vector<Plugin::Result>, NotificationType>> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        
        std::atomic<ProcessState> mRenderingState {ProcessState::available};
        std::future<std::tuple<juce::Image, NotificationType>> mRenderingProcess;
        
        AtomicManager<Processor> mProcessorManager;
        AtomicManager<juce::AudioFormatReader> mAudioFormatReaderManager;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
