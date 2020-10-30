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
    anlStrongAssert(mAudioFormatReader->sampleRate > 0.0);
    auto const currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    auto const sourceSampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : currentSampleRate;
    
    mResamplingAudioSource.setResamplingRatio(sourceSampleRate / currentSampleRate);
    mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, currentSampleRate);
}

void Document::AudioReader::Source::releaseResources()
{
    mResamplingAudioSource.releaseResources();
}

void Document::AudioReader::Source::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto lastPosition = getNextReadPosition();
    if(mReadPosition != lastPosition)
    {
        lastPosition = mReadPosition;
        mAudioFormatReaderSource.setNextReadPosition(lastPosition);
        mResamplingAudioSource.flushBuffers();
    }
    mResamplingAudioSource.getNextAudioBlock(bufferToFill);
    if(!mReadPosition.compare_exchange_weak(lastPosition, getNextReadPosition()))
    {
        mAudioFormatReaderSource.setNextReadPosition(mReadPosition.load());
        mResamplingAudioSource.flushBuffers();
    }
}

void Document::AudioReader::Source::setNextReadPosition(juce::int64 newPosition)
{
    mReadPosition = newPosition;
    mAudioFormatReaderSource.setNextReadPosition(newPosition);
    mResamplingAudioSource.flushBuffers();
}

juce::int64 Document::AudioReader::Source::getNextReadPosition() const
{
    return mAudioFormatReaderSource.getNextReadPosition();
}

juce::int64 Document::AudioReader::Source::getTotalLength() const
{
    return mAudioFormatReaderSource.getTotalLength();
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

Document::AudioReader::AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
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
            case Attribute::isLooping:
            {
                mIsLooping.store(acsr.getModel().isLooping);
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
    
    mReceiver.onSignal = [&](Accessor& acsr, Signal signal, juce::var value)
    {
        juce::ignoreUnused(acsr);
        auto getLastPosition = [&]() -> juce::int64
        {
            auto instance = mSourceManager.getInstance();
            if(instance != nullptr)
            {
                return instance->getTotalLength();
            }
            return 0;
        };
        
        switch (signal)
        {
            case Signal::movePlayhead:
            {
                mReadPosition.store(static_cast<bool>(value) ? getLastPosition() : 0);
            }
                break;
            case Signal::togglePlayback:
            {
                if(static_cast<bool>(value))
                {
                    auto expected = getLastPosition();
                    mReadPosition.compare_exchange_strong(expected, 0);
                    mIsPlaying.store(true);
                    startTimer(50);
                }
                else
                {
                    stopTimer();
                    mIsPlaying.store(false);
                }
            }
                break;
            case Signal::playheadPosition:
            {
                mReadPosition = static_cast<juce::int64>(value);
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
            instance->getNextAudioBlock(bufferToFill);
            mReadPosition = instance->getNextReadPosition();
            
            auto const endPosition = instance->getTotalLength();
            if(mIsLooping && mReadPosition >= endPosition)
            {
                instance->setNextReadPosition(0);
                mReadPosition = 0;
            }

            if(mReadPosition >= endPosition)
            {
                mReadPosition = endPosition;
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

void Document::AudioReader::handleAsyncUpdate()
{
    mAccessor.sendSignal(Signal::togglePlayback, {false}, juce::NotificationType::sendNotificationSync);
}

void Document::AudioReader::timerCallback()
{
    mAccessor.sendSignal(Signal::playheadPosition, {mReadPosition.load()}, juce::NotificationType::sendNotificationSync);
}

ANALYSE_FILE_END
