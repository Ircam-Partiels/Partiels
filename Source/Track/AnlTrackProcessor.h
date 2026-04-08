#pragma once

#include "../Plugin/AnlPluginProcessor.h"
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

        std::function<void(juce::Result const&, Results const&)> onAnalysisEnded = nullptr;
        std::function<void(void)> onAnalysisAborted = nullptr;

    private:
        void abortAnalysis();

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        using ProcessResult = std::tuple<juce::Result, Results>;
        static ProcessResult runWaveformAnalysis(juce::AudioFormatReader& reader, std::function<bool(float)> callback);
        static ProcessResult runPluginAnalysis(Plugin::Processor& processor, Results const& input, std::function<bool(float)> callback);

        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        std::atomic<bool> mShouldAbort{false};
        std::future<std::tuple<juce::Result, Results>> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
} // namespace Track

ANALYSE_FILE_END
