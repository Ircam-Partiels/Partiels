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
        // [input.identifier, [result, thresholds]]
        using InputStates = std::map<juce::String, std::pair<Results, std::vector<std::optional<float>>>>;

        //! @brief The results of a feature analyzed by the processor
        struct FeatureResult
        {
            std::string feature;             //!< The identifier of the feature
            Results results;                 //!< The results of the feature
            Plugin::Description description; //!< The description of the plugin for the feature
        };

        Processor() = default;
        ~Processor() override;

        //! @brief Runs the analysis
        //! @details The features are the identifiers of the outputs analyzed during the same processing (e.g. the
        //! voices of an output), if the list is empty, only the feature of the key of the track is analyzed.
        bool runAnalysis(Accessor const& accessor, juce::AudioFormatReader& reader, InputStates inputStates, std::vector<std::string> const& features = {});
        void stopAnalysis();
        bool isRunning() const;
        float getAdvancement() const;

        std::function<void(juce::Result const&, std::vector<FeatureResult> const&)> onAnalysisEnded = nullptr;
        std::function<void(void)> onAnalysisAborted = nullptr;

    private:
        void abortAnalysis(std::unique_lock<std::mutex>& lock);

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        using ProcessResult = std::tuple<juce::Result, Results>;
        using PluginProcessResult = std::tuple<juce::Result, std::vector<Results>>;
        static ProcessResult runWaveformAnalysis(juce::AudioFormatReader& reader, std::function<bool(float)> callback);
        static PluginProcessResult runPluginAnalysis(Plugin::Processor& processor, InputStates const& inputStates, std::function<bool(float)> callback);
        static ProcessResult createEmptyProcessResult(juce::Result result);
        static PluginProcessResult createEmptyPluginProcessResult(juce::Result result);
        static juce::Result createError(juce::String const& reason);

        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        std::atomic<bool> mShouldAbort{false};
        std::future<std::tuple<juce::Result, std::vector<FeatureResult>>> mAnalysisProcess;
        std::mutex mAnalysisMutex;
        std::atomic<float> mAdvancement{0.0f};
        Chrono mChrono{"Track"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
    };
} // namespace Track

ANALYSE_FILE_END
