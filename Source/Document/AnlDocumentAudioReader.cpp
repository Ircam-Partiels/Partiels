#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

Document::AudioReader::Source::Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAudioFormatReader(std::move(audioFormatReader))
, mAudioFormatReaderSource(mAudioFormatReader.get(), false)
, mResamplingAudioSource(&mAudioFormatReaderSource, false, static_cast<int>(mAudioFormatReader->numChannels))
{
    anlStrongAssert(mAudioFormatReader != nullptr);
}

void Document::AudioReader::Source::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlStrongAssert(sampleRate > 0.0);
    if(mAudioFormatReader->sampleRate > 0.0)
    {
        mResamplingAudioSource.setResamplingRatio(mAudioFormatReader->sampleRate / (sampleRate > 0.0 ? sampleRate : 44100.0));
        mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
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
    auto const ratio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(ratio > 0.0);
    mAudioFormatReaderSource.setNextReadPosition(static_cast<juce::int64>(static_cast<double>(newPosition) * (ratio > 0.0 ? ratio : 1.0)));
    mResamplingAudioSource.flushBuffers();
}

juce::int64 Document::AudioReader::Source::getNextReadPosition() const
{
    auto const ratio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(ratio > 0.0);
    return static_cast<juce::int64>(static_cast<double>(mAudioFormatReaderSource.getNextReadPosition()) / (ratio > 0.0 ? ratio : 1.0));
}

juce::int64 Document::AudioReader::Source::getTotalLength() const
{
    auto const ratio = mResamplingAudioSource.getResamplingRatio();
    anlStrongAssert(ratio > 0.0);
    return static_cast<juce::int64>(static_cast<double>(mAudioFormatReaderSource.getTotalLength()) / (ratio > 0.0 ? ratio : 1.0));
}

bool Document::AudioReader::Source::isLooping() const
{
    anlStrongAssert(false && "this method should never be used");
    return false;
}

void Document::AudioReader::Source::setLooping(bool shouldLoop)
{
    anlStrongAssert(false && "this method should never be used");
    juce::ignoreUnused(shouldLoop);
}

Document::AudioReader::AudioReader(juce::AudioFormatManager& audioFormatManager, Accessor& accessor)
: mAudioFormatManager(audioFormatManager)
, mAccessor(accessor)
{
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
    
    mReceiver.onSignal = [&](Accessor& acsr, Signal signal, juce::var value)
    {
        switch (signal)
        {
            case Signal::movePlayhead:
            {
                mReadPosition.store(static_cast<bool>(value) ? getTotalLength() : 0);
            }
                break;
            case Signal::togglePlayback:
            {
                mIsPlaying.store(static_cast<bool>(value));
            }
                break;
            case Signal::toggleLooping:
            {
                auto const rate = mSampleRate;
                auto const loopStart = static_cast<juce::int64>(std::floor(acsr.getModel().loop.getStart() * rate));
                auto const loopEnd = static_cast<juce::int64>(std::floor(acsr.getModel().loop.getEnd() * rate));
                mLoop.store(static_cast<bool>(value) ? juce::Range<juce::int64>{loopStart, loopEnd} : juce::Range<juce::int64>{0, 0});
            }
                break;
        }
    };
    mAccessor.addReceiver(mReceiver);
}

Document::AudioReader::~AudioReader()
{
    mAccessor.removeReceiver(mReceiver);
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
    if(mIsPlaying.load() == true)
    {
        auto instance = mSourceManager.getInstance();
        if(instance != nullptr)
        {
            auto expectedPosition = instance->getNextReadPosition();
            auto const nextReadPosition = expectedPosition + static_cast<juce::int64>(bufferToFill.numSamples);
            if(!mReadPosition.compare_exchange_strong(expectedPosition, nextReadPosition))
            {
                instance->setNextReadPosition(mReadPosition.load());
                mReadPosition += static_cast<juce::int64>(bufferToFill.numSamples);
            }
            
            instance->getNextAudioBlock(bufferToFill);
            
            if(mReadPosition >= instance->getTotalLength())
            {
                mIsPlaying.store(false);
                triggerAsyncUpdate();
            }
        }
    }
    else
    {
        bufferToFill.buffer->clear();
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
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        return instance->isLooping();
    }
    return false;
}

void Document::AudioReader::setLooping(bool shouldLoop)
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        return instance->setLooping(shouldLoop);
    }
}

void Document::AudioReader::handleAsyncUpdate()
{
    mAccessor.sendSignal(Signal::togglePlayback, {false}, juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END
