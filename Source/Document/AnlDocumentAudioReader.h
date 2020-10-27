#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlAtomicManager.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The audio reader of a document
    class AudioReader
    : public juce::AudioSource
    , private juce::AsyncUpdater
    , private juce::Timer
    {
    public:
        using Attribute = Model::Attribute;
        using Signal = Model::Signal;
        
        AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager);
        ~AudioReader() override;

        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;
        
    private:
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
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

        private:
            std::unique_ptr<juce::AudioFormatReader> mAudioFormatReader;
            juce::AudioFormatReaderSource mAudioFormatReaderSource;
            juce::ResamplingAudioSource mResamplingAudioSource;
            std::atomic<juce::int64> mReadPosition {0};
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;
        double mSampleRate = 44100.0;
        int mSamplesPerBlockExpected = 512;
        
        Tools::AtomicManager<Source> mSourceManager;
        std::atomic<bool> mIsPlaying {false};
        std::atomic<bool> mIsLooping {false};
        std::atomic<juce::int64> mReadPosition {0};
        
        JUCE_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END
