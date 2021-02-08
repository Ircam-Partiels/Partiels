#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Director
    : private juce::AsyncUpdater
    {
    public:
        Director(Accessor& accessor, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;
        
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
        
    private:
        
        void runAnalysis(NotificationType const notification);
        void runRendering(NotificationType const notification);
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
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
