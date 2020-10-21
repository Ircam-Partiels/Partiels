#include "AnlDocumentReader.h"

ANALYSE_FILE_BEGIN

Document::Reader::Reader(juce::AudioFormatManager& audioFormatManager, Accessor& accessor)
: mAudioFormatManager(audioFormatManager)
, mAccessor(accessor)
{
    using Attribute = Model::Attribute;
    mListener.onChanged = [&](Accessor& acsr, Attribute attribute)
    {
        switch (attribute)
        {
            case Attribute::file:
            {
                auto const file = acsr.getModel().file;
                auto* audioFormat = mAudioFormatManager.findFormatForFileExtension(file.getFileExtension());
                if(audioFormat == nullptr)
                {
                    return;
                }
                auto audioFormatReader = std::shared_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                if(audioFormatReader == nullptr)
                {
                    return;
                }
            }
                break;
            case Attribute::analyzers:
                break;
            case Attribute::loop:
                break;
        }
    };
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
}

Document::Reader::~Reader()
{
    
}

void Document::Reader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    
}

void Document::Reader::releaseResources()
{
    
}

void Document::Reader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    
}

void Document::Reader::setNextReadPosition(juce::int64 newPosition)
{
    
}

juce::int64 Document::Reader::getNextReadPosition() const
{
    return 0;
}

juce::int64 Document::Reader::getTotalLength() const
{
    return 0;
}

bool Document::Reader::isLooping() const
{
    
}

void Document::Reader::setLooping(bool shouldLoop)
{
    
}

ANALYSE_FILE_END
