#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::AudioFormatReader> Document::createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager, AlertType alertType)
{
    using AlertIconType = juce::AlertWindow::AlertIconType;
    auto const errorMessage = juce::translate("Audio format reader cannot be loaded!");
    
    auto const file = accessor.getAttr<AttrType::file>();
    if(file == juce::File())
    {
        return nullptr;
    }
    
    if(auto* format = audioFormatManager.findFormatForFileExtension(file.getFileExtension()))
    {
        if(auto* audioFormatReader = format->createMemoryMappedReader(file))
        {
            anlWeakAssert(audioFormatReader->sampleRate > 0.0);
            if(audioFormatReader->sampleRate > 0.0)
            {
                audioFormatReader->mapEntireFile();
                return std::unique_ptr<juce::AudioFormatReader>(audioFormatReader);
            }
        }
    }
   
    auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(file));
    if(audioFormatReader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The audio format reader cannot be allocated for the input stream FLNAME.").replace("FLNAME", file.getFullPathName()));
        }
        return nullptr;
    }
    return audioFormatReader;
}

Document::AudioReader::AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mTransportAudioReader(mAccessor.getAcsr<AcsrType::transport>())
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::file:
            {
                auto const file = acsr.getAttr<AttrType::file>();
                if(file == juce::File{})
                {
                    mTransportAudioReader.setAudioFormatReader(nullptr);
                    return;
                }
                mTransportAudioReader.setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window));
            }
                break;
            case AttrType::layoutVertical:
            case AttrType::layout:
            case AttrType::expanded:
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::AudioReader::~AudioReader()
{
    mAccessor.removeListener(mListener);
}

void Document::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mTransportAudioReader.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Document::AudioReader::releaseResources()
{
    mTransportAudioReader.releaseResources();
}

void Document::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mTransportAudioReader.getNextAudioBlock(bufferToFill);
}

ANALYSE_FILE_END
