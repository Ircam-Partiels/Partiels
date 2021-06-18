#pragma once

#include "../Document/AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AudioReader
    : private juce::AudioSource
    {
    public:
        AudioReader();
        ~AudioReader() override;

    private:
        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;

        Document::AudioReader mDocumentAudioReader;
        juce::AudioSourcePlayer mAudioSourcePlayer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
} // namespace Application

ANALYSE_FILE_END
