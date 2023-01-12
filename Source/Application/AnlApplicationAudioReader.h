#pragma once

#include "../Document/AnlDocumentAudioReader.h"
#include "AnlApplicationModel.h"

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

        void updateBuffer();

        Accessor::Listener mListener{typeid(*this).name()};
        Document::AudioReader mDocumentAudioReader;
        juce::AudioSourcePlayer mAudioSourcePlayer;
        int mBlockSize = 0;

        audio_spin_mutex mMutex;
        juce::AudioBuffer<float> mBuffer;
        std::vector<std::vector<bool>> mMatrix;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
} // namespace Application

ANALYSE_FILE_END
