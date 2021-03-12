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

        double getSampleRate() const;
        
        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;
        
    private:
        
        // juce::Timer
        void timerCallback() override;
        
        class Source
        : public juce::PositionableAudioSource
        {
        public:
            Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
            ~Source() override = default;
            
            // juce::AudioSource
            void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
            void releaseResources() override;
            void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;
            
            // juce::PositionableAudioSource
            void setNextReadPosition(juce::int64 newPosition) override;
            juce::int64 getNextReadPosition() const override;
            juce::int64 getTotalLength() const override;
            bool isLooping() const override;
            void setLooping(bool shouldLoop)  override;
            
            void setGain(float gain);

        private:
            std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
            juce::AudioFormatReaderSource mAudioFormatReaderSource;
            std::atomic<juce::int64> mReadPosition {0};
            juce::LinearSmoothedValue<float> mVolume;
            std::atomic<float> mVolumeTargetValue;
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor::Listener mListener;
        double mSampleRate = 44100.0;
        int mSamplesPerBlockExpected = 512;
        
        AtomicManager<Source> mSourceManager;
        std::atomic<bool> mIsPlaying {false};
        std::atomic<bool> mIsLooping {false};
        std::atomic<juce::int64> mReadPosition {0};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END
