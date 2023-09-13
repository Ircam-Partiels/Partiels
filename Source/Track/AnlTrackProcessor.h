#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Processor
    : private juce::AsyncUpdater
    {
    public:
        Processor() = default;
        ~Processor() override;

        std::optional<Plugin::Description> runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader, Results input);
        void stopAnalysis();
        bool isRunning() const;
        float getAdvancement() const;

        std::function<void(Results const& results)> onAnalysisEnded = nullptr;
        std::function<void(void)> onAnalysisAborted = nullptr;

    private:
        void abortAnalysis();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // clang-format off
        enum class ProcessState
        {
              available
            , aborted
            , running
            , ended
        };
        // clang-format on

        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;

        std::atomic<ProcessState> mAnalysisState{ProcessState::available};
        std::atomic<bool> mShouldAbort{false};
        std::future<Results> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track", "processor analysis ended"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
} // namespace Track

ANALYSE_FILE_END
