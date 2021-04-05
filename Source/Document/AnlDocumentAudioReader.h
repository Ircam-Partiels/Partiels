#pragma once

#include "AnlDocumentModel.h"
#include "../Transport/AnlTransportAudioReader.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    std::unique_ptr<juce::AudioFormatReader> createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager, AlertType alertType);
    
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
        
        Accessor::Listener mListener;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioReader)
    };
}

ANALYSE_FILE_END
