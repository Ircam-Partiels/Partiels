#pragma once

#include "AnlDocumentModel.h"
#include "../Tools/AnlAtomicManager.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The audio reader of a document
    class AudioReader
    : public juce::PositionableAudioSource
    {
    public:
        AudioReader(juce::AudioFormatManager& audioFormatManager, Accessor& accessor);
        ~AudioReader() override;

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
            juce::AudioFormatReaderSource mAudioFormatReaderSource {mAudioFormatReader.get(), false};
            juce::AudioTransportSource mAudioTransportSource;
            juce::ResamplingAudioSource mResamplingAudioSource {&mAudioFormatReaderSource, false, static_cast<int>(mAudioFormatReader->numChannels)};
            std::atomic<juce::int64> mPosition {0};
        };
        
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor& mAccessor;
        Accessor::Listener mListener;
        double mSampleRate = 44100.0;
        int mSamplesPerBlockExpected = 512;
        
        Tools::AtomicManager<Source> mSourceManager;
        std::atomic<juce::Range<double>> mLoop;
        
        JUCE_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END
