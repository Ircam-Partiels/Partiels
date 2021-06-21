#pragma once

#include "../Transport/AnlTransportAudioReader.h"
#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    std::tuple<std::unique_ptr<juce::AudioFormatReader>, juce::StringArray> createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager);

    //! @brief The audio reader of a document
    class AudioReader
    : public juce::AudioSource
    {
    public:
        AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager);
        ~AudioReader() override;

        // juce::AudioSource
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill) override;

    private:
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        Transport::AudioReader mTransportAudioReader;

        Accessor::Listener mListener{typeid(*this).name()};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
} // namespace Document

ANALYSE_FILE_END
