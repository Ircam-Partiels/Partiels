#pragma once
#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AudioReader
    : private juce::AudioSource
    {
    public:
        
        static const int sMaxIONumber = 64;
        
        AudioReader();
        ~AudioReader() override;
        
    private:
        
        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;
        
        juce::AudioTransportSource mAudioTransportSource;
        juce::AudioSourcePlayer mAudioSourcePlayer;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END

