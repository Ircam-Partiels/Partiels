#pragma once

#include "AnlTrackModel.h"
#include "../Plugin/AnlPluginListScanner.h"

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
        
        std::function<void(std::vector<Plugin::Result> const& results)> onAnalysisEnded = nullptr;
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
        std::future<std::vector<Plugin::Result>> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        Chrono mChrono {"Track", "processor analysis ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
}

ANALYSE_FILE_END
