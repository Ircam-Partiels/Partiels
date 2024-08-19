#pragma once

#include "MiscTransportModel.h"

MISC_FILE_BEGIN

namespace Transport
{
    class AudioReader
    : public juce::AudioSource
    , private juce::Timer
    {
    public:
        AudioReader(Accessor& accessor);
        ~AudioReader() override;

        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader);

        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;

    private:
        // juce::Timer
        void timerCallback() override;

        class Source
        : public juce::AudioSource
        {
        public:
            Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
            ~Source() override = default;

            // juce::AudioSource
            void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
            void releaseResources() override;
            void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;

            void setPlaying(bool shouldPlay);
            void setGain(float gain);
            void setStartPlayheadPosition(double position);
            void setLooping(bool shouldLoop);
            void setLoopRange(juce::Range<double> const& loopRange);
            void setStopAtLoopEnd(bool shouldStop);

            double getSampleRate() const;
            bool isPlaying() const;
            double getReadPlayheadPosition() const;

        private:
            std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
            juce::AudioFormatReaderSource mAudioFormatReaderSource;

            std::atomic<bool> mIsPlaying{false};
            std::atomic<juce::int64> mReadPosition{0};
            std::atomic<juce::int64> mStartPosition{0};
            std::atomic<bool> mIsLooping{false};
            std::atomic<bool> mStopAtLoopEnd{false};
            std::atomic<juce::Range<juce::int64>> mLoopRange{};
            juce::LinearSmoothedValue<float> mVolume;
            std::atomic<float> mVolumeTargetValue;
        };

        class ResamplingSource
        : public juce::AudioSource
        {
        public:
            ResamplingSource(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, int numChannels);
            ~ResamplingSource() override = default;

            // juce::AudioSource
            void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
            void releaseResources() override;
            void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;

            void setPlaying(bool shouldPlay);
            void setGain(float gain);
            void setStartPlayheadPosition(double position);
            void setLooping(bool shouldLoop);
            void setLoopRange(juce::Range<double> const& loopRange);
            void setStopAtLoopEnd(bool shouldStop);

            bool isPlaying() const;
            double getReadPlayheadPosition() const;

        private:
            Source mSource;
            juce::ResamplingAudioSource mResamplingAudioSource;
        };

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        double mSampleRate = 44100.0;
        int mSamplesPerBlockExpected = 512;
        audio_spin_mutex mMutex;
        std::unique_ptr<ResamplingSource> mSource;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
} // namespace Transport

MISC_FILE_END
