#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class Processor
    {
    public:
        //! @brief The results of the features [features[channels[results]]]
        using Results = std::vector<std::vector<std::vector<Result>>>;

        //! @brief Creates a processor
        //! @details The features are the identifiers of the outputs analyzed during the same processing (e.g. the
        //! voices of an output), if the list is empty, only the feature of the key is analyzed. The features are
        //! resolved with the parameters of the state and the numbers of voices because the outputs can depend on them.
        //! @param key The key of the plugin
        //! @param state The state of the plugin
        //! @param voiceCounts The numbers of voices of the outputs
        //! @param audioFormatReader The audio reader
        //! @param features The identifiers of the outputs to analyze
        //! @return The processor (or nullptr if the plugin cannot be allocated)
        static std::unique_ptr<Processor> create(Key const& key, State const& state, VoiceCounts const& voiceCounts, juce::AudioFormatReader& audioFormatReader, std::vector<std::string> const& features = {});

        ~Processor() = default;

        //! @brief The results of the inputs [channels[inputs[results]]]
        using InputResults = std::vector<std::vector<std::vector<Result>>>;

        //! @brief Prepares the analysis
        //! @details The plugins are initialized, then the results of the inputs are set as pre-computing features
        //! before retrieving the output descriptors, because the output descriptors can depend on the inputs (e.g. the
        //! number of bins and their names) and the Vamp plugin adapter caches them until the next initialization (or
        //! the next change of a parameter or of the program).
        //! @param results The results to prepare
        //! @param inputResults The results of the inputs (one channel for all the plugins or one channel per plugin)
        //! @return The result of the preparation
        juce::Result prepareToAnalyze(Results& results, InputResults const& inputResults = {});
        std::tuple<juce::Result, bool> performNextAudioBlock(Results& results);
        float getAdvancement() const;

        //! @brief Gets the number of features analyzed by the processor
        size_t getNumFeatures() const;
        //! @brief Gets the description of a feature analyzed by the processor
        Description getDescription(size_t featureIndex = 0_z) const;
        std::vector<Input> getInputs() const;
        //! @brief Gets the output of a feature analyzed by the processor
        Output getOutput(size_t featureIndex = 0_z) const;

    private:
        class CircularReader
        {
        public:
            CircularReader(juce::AudioFormatReader& audioFormatReader, size_t blockSize, size_t stepSize);
            ~CircularReader() = default;

            size_t getNumChannels() const;
            juce::int64 getOffsetInSamples() const;
            juce::int64 getLengthInSamples() const;
            double getSampleRate() const;
            bool hasReachedEnd() const;
            juce::int64 getPosition() const;
            float const** getNextBlock();

        private:
            int const mBlocksize;
            int const mStepSize;
            juce::int64 const mOffset;
            juce::AudioFormatReader& mAudioFormatReader;
            juce::AudioBuffer<float> mBuffer;
            int mBufferPosition{0};
            juce::int64 mReaderPosition{0};
            juce::int64 mPosition{0};
            std::vector<float const*> mOutputBuffer;
        };

        Processor(juce::AudioFormatReader& audioFormatReader, std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins, Key const& key, std::vector<std::string> const& features, std::vector<size_t> const& featureIndices, State const& state, VoiceCounts const& voiceCounts);

        juce::Result setPrecomputingResults(InputResults const& inputResults);

        std::vector<std::unique_ptr<Ive::PluginWrapper>> mPlugins;
        CircularReader mCircularReader;
        Key const mKey;
        std::vector<std::string> const mFeatures;
        std::vector<size_t> const mFeatureIndices;
        State const mState;
        VoiceCounts const mVoiceCounts;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)

    public:
        class CircularReaderUnitTest;
        class FeaturesUnitTest;
    };
} // namespace Plugin

ANALYSE_FILE_END
