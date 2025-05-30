#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class Processor
    {
    public:
        static std::unique_ptr<Processor> create(Key const& key, State const& state, juce::AudioFormatReader& audioFormatReader);

        ~Processor() = default;

        bool prepareToAnalyze(std::vector<std::vector<Result>>& results);
        bool setPrecomputingResults(std::vector<std::vector<Result>> const& results);
        bool performNextAudioBlock(std::vector<std::vector<Result>>& results);
        float getAdvancement() const;

        Description getDescription() const;
        Input getInput() const;
        Output getOutput() const;

    private:
        class CircularReader
        {
        public:
            CircularReader(juce::AudioFormatReader& audioFormatReader, size_t blockSize, size_t stepSize);
            ~CircularReader() = default;

            juce::int64 getLengthInSamples() const;
            double getSampleRate() const;
            bool hasReachedEnd() const;
            juce::int64 getPosition() const;
            float const** getNextBlock();

        private:
            int const mBlocksize;
            int const mStepSize;
            juce::AudioFormatReader& mAudioFormatReader;
            juce::AudioBuffer<float> mBuffer;
            int mBufferPosition{0};
            juce::int64 mReaderPosition{0};
            juce::int64 mPosition{0};
            std::vector<float const*> mOutputBuffer;
        };

        Processor(juce::AudioFormatReader& audioFormatReader, std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins, Key const& key, size_t const feature, State const& state);

        std::vector<std::unique_ptr<Ive::PluginWrapper>> mPlugins;
        CircularReader mCircularReader;
        Key const mKey;
        size_t const mFeature;
        State const mState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)

    public:
        class CircularReaderUnitTest;
    };
} // namespace Plugin

ANALYSE_FILE_END
