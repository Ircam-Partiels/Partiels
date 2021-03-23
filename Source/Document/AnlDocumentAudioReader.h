#pragma once

#include "AnlDocumentModel.h"
#include "../Misc/AnlAtomicManager.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    std::unique_ptr<juce::AudioFormatReader> createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager, AlertType alertType);
    
    //! @brief The audio reader of a document
    class AudioReader
    : public juce::AudioSource
    , private juce::Timer
    {
    public:
        AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager);
        ~AudioReader() override;
        
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
            void setLooping(bool shouldLoop);
            void setGain(float gain);
            
            double getSampleRate() const;
            bool isPlaying() const;
            double getReadPlayheadPosition() const;

        private:
            
            std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
            juce::AudioFormatReaderSource mAudioFormatReaderSource;
            
            std::atomic<bool> mIsPlaying {false};
            std::atomic<bool> mIsLooping {false};
            std::atomic<juce::int64> mReadPosition {0};
            std::atomic<juce::int64> mStartPosition {0};
            juce::LinearSmoothedValue<float> mVolume;
            std::atomic<float> mVolumeTargetValue;
        };
        
        class ResamplingSource
        : public juce::AudioSource
        {
        public:
            
            ResamplingSource(std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
            ~ResamplingSource() override = default;
            
            // juce::AudioSource
            void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
            void releaseResources() override;
            void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;
            
            void setPlaying(bool shouldPlay);
            void setLooping(bool shouldLoop);
            void setGain(float gain);
            
            bool isPlaying() const;
            double getReadPlayheadPosition() const;
            
        private:
            Source mSource;
            juce::ResamplingAudioSource mResamplingAudioSource {&mSource, false};
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor::Listener mListener;
        double mSampleRate = 44100.0;
        int mSamplesPerBlockExpected = 512;
        
        AtomicManager<ResamplingSource> mSourceManager;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END
