#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The audio reader of a document
    class Reader
    : public juce::PositionableAudioSource
    {
    public:
        Reader(juce::AudioFormatManager& audioFormatManager, Accessor& accessor);
        ~Reader() override;

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
        juce::AudioFormatManager& mAudioFormatManager;
        Accessor& mAccessor;
        Accessor::Listener mListener;
        std::atomic<juce::Range<double>> mLoop;
        
        std::shared_ptr<juce::AudioFormatReader> mAudioFormatReader {nullptr};
        
        JUCE_LEAK_DETECTOR(Reader)
    };
}

ANALYSE_FILE_END
