#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

Document::AudioReader::Source::Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAudioFormatReader(std::move(audioFormatReader))
{
    anlStrongAssert(audioFormatReader != nullptr);
}

void Document::AudioReader::Source::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlStrongAssert(sampleRate > 0.0);
    auto const resamplingRatio = mAudioFormatReader->sampleRate / (sampleRate > 0.0 ? sampleRate : 44100.0);
    mResamplingAudioSource.setResamplingRatio(resamplingRatio);
    mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, (sampleRate > 0.0 ? sampleRate : 44100.0));
}

void Document::AudioReader::Source::releaseResources()
{
    mResamplingAudioSource.releaseResources();
}

void Document::AudioReader::Source::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mResamplingAudioSource.getNextAudioBlock(bufferToFill);
}

void Document::AudioReader::Source::setNextReadPosition(juce::int64 newPosition)
{
    auto const resamplingRatio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(resamplingRatio > 0.0);
    mAudioFormatReaderSource.setNextReadPosition(0);
    mPosition = static_cast<juce::int64>(static_cast<double>(newPosition) / (resamplingRatio > 0.0 ? resamplingRatio : 1.0));
}

juce::int64 Document::AudioReader::Source::getNextReadPosition() const
{
    auto const resamplingRatio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(resamplingRatio > 0.0);
    return static_cast<juce::int64>(static_cast<double>(mPosition) * (resamplingRatio > 0.0 ? resamplingRatio : 1.0));
}

juce::int64 Document::AudioReader::Source::getTotalLength() const
{
    auto const resamplingRatio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(resamplingRatio > 0.0);
    return static_cast<juce::int64>(static_cast<double>(mAudioFormatReaderSource.getTotalLength()) * (resamplingRatio > 0.0 ? resamplingRatio : 1.0));
}

bool Document::AudioReader::Source::isLooping() const { return false; }
void Document::AudioReader::Source::setLooping(bool shouldLoop)  { juce::ignoreUnused(shouldLoop); }

Document::AudioReader::AudioReader(juce::AudioFormatManager& audioFormatManager, Accessor& accessor)
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
                    mSourceManager.setInstance({});
                    return;
                }
                auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormat->createReaderFor(file.createInputStream().release(), true));
                if(audioFormatReader == nullptr)
                {
                    JUCE_COMPILER_WARNING("error here?");
                    mSourceManager.setInstance({});
                    return;
                }
                auto source = std::make_shared<Source>(std::move(audioFormatReader));
                if(source != nullptr)
                {
                    source->prepareToPlay(mSamplesPerBlockExpected, mSampleRate);
                }
                mSourceManager.setInstance(source);
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

Document::AudioReader::~AudioReader()
{
    mAccessor.removeListener(mListener);
}

void Document::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mSamplesPerBlockExpected = samplesPerBlockExpected;
    mSampleRate = sampleRate;
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Document::AudioReader::releaseResources()
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->releaseResources();
    }
}

void Document::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->getNextAudioBlock(bufferToFill);
    }
}

void Document::AudioReader::setNextReadPosition(juce::int64 newPosition)
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->setNextReadPosition(newPosition);
    }
}

juce::int64 Document::AudioReader::getNextReadPosition() const
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        return instance->getNextReadPosition();
    }
    return 0;
}

juce::int64 Document::AudioReader::getTotalLength() const
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        return instance->getTotalLength();
    }
    return 0;
}

bool Document::AudioReader::isLooping() const
{
    
}

void Document::AudioReader::setLooping(bool shouldLoop)
{
    
}

ANALYSE_FILE_END
