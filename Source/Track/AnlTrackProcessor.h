#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Processor
    : private juce::AsyncUpdater
    {
    public:
        using Result = std::optional<std::tuple<WarningType, juce::String, Plugin::Description, Plugin::State>>;
        
        Processor() = default;
        ~Processor() override;
        
        Result runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader);
        void stopAnalysis();
        float getAdvancement() const;
        
        std::function<void(std::shared_ptr<std::vector<Plugin::Result>> results)> onAnalysisEnded = nullptr;
        std::function<void(void)> onAnalysisAborted = nullptr;
        
    private:
        void abortAnalysis();
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
       
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        
        std::atomic<ProcessState> mAnalysisState {ProcessState::available};
        std::future<std::shared_ptr<std::vector<Plugin::Result>>> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        std::atomic<float> mAdvancement {0.0f};
        Chrono mChrono {"Track", "processor analysis ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}

ANALYSE_FILE_END
