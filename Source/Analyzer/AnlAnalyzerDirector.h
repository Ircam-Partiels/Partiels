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
        
        void createProcessor(NotificationType const notification);
        void runAnalysis(NotificationType const notification);
        void updateZoomRange(NotificationType const notification);
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        enum class AnalysisState
        {
              available
            , abort
            , run
        };
        
        Accessor& mAccessor;
        std::atomic<AnalysisState> mState {AnalysisState::available};
        std::future<std::tuple<std::vector<Analyzer::Result>, NotificationType>> mFuture;
        
        AtomicManager<Processor> mProcessorManager;
        AtomicManager<juce::AudioFormatReader> mAudioFormatReaderManager;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
