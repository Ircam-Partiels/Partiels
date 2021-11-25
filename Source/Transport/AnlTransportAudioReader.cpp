#include "AnlTransportAudioReader.h"

ANALYSE_FILE_BEGIN

Transport::AudioReader::Source::Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAudioFormatReader(std::move(audioFormatReader))
, mAudioFormatReaderSource(mAudioFormatReader.get(), false)
{
    anlStrongAssert(mAudioFormatReader != nullptr);
}

void Transport::AudioReader::Source::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlStrongAssert(sampleRate > 0.0 && mAudioFormatReader->sampleRate > 0.0);
    anlStrongAssert(std::abs(mAudioFormatReader->sampleRate - sampleRate) < 0.001);
    sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    mAudioFormatReaderSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    mVolume.reset(mAudioFormatReader->sampleRate, static_cast<double>(samplesPerBlockExpected) / sampleRate);
    mVolume.setCurrentAndTargetValue(mVolumeTargetValue.load());
}

void Transport::AudioReader::Source::releaseResources()
{
    mAudioFormatReaderSource.releaseResources();
}

void Transport::AudioReader::Source::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    anlStrongAssert(buffer != nullptr);
    if(buffer == nullptr)
    {
        return;
    }

    mVolume.setTargetValue(mVolumeTargetValue.load());

    auto numSamplesToProceed = bufferToFill.numSamples;
    auto outputPosition = bufferToFill.startSample;
    while(numSamplesToProceed > 0)
    {
        if(mIsPlaying.load())
        {
            auto const currentReadPosition = mReadPosition.load();
            auto const loopRange = mLoopRange.load();
            auto const canLoop = !loopRange.isEmpty() && currentReadPosition < loopRange.getEnd();
            auto const stopAtLoopEnd = mStopAtLoopEnd.load() && canLoop;
            auto const isLooping = mIsLooping && canLoop;
            auto const endPosition = (isLooping || stopAtLoopEnd) ? loopRange.getEnd() : mAudioFormatReaderSource.getTotalLength() - 1;
            auto const numRemainingSamples = static_cast<int>(std::min(endPosition - currentReadPosition, static_cast<juce::int64>(numSamplesToProceed)));
            juce::AudioSourceChannelInfo tempBuffer(buffer, outputPosition, numRemainingSamples);

            mAudioFormatReaderSource.setNextReadPosition(currentReadPosition);
            mAudioFormatReaderSource.getNextAudioBlock(tempBuffer);
            auto const nextReadPosition = mAudioFormatReaderSource.getNextReadPosition();
            if(nextReadPosition >= endPosition)
            {
                auto const startPosition = isLooping ? loopRange.getStart() : mStartPosition.load();
                mReadPosition = startPosition;
                mAudioFormatReaderSource.setNextReadPosition(startPosition);
                mIsPlaying.store(mIsPlaying.load() && isLooping);
            }
            else
            {
                mReadPosition = nextReadPosition;
            }

            numSamplesToProceed -= numRemainingSamples;
            outputPosition += numRemainingSamples;
        }
        else
        {
            buffer->clear(outputPosition, numSamplesToProceed);
            outputPosition += numSamplesToProceed;
            numSamplesToProceed = 0;
        }
    }

    juce::AudioBuffer<float> tempBuffer(buffer->getArrayOfWritePointers(), buffer->getNumChannels(), bufferToFill.startSample, bufferToFill.numSamples);
    mVolume.applyGain(tempBuffer, bufferToFill.numSamples);
}

double Transport::AudioReader::Source::getSampleRate() const
{
    return mAudioFormatReader->sampleRate;
}

bool Transport::AudioReader::Source::isPlaying() const
{
    return mIsPlaying.load();
}

double Transport::AudioReader::Source::getReadPlayheadPosition() const
{
    auto const sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    return static_cast<double>(mReadPosition.load()) / sampleRate;
}

void Transport::AudioReader::Source::setPlaying(bool shouldPlay)
{
    mReadPosition.store(mStartPosition.load());
    mIsPlaying = shouldPlay;
}

void Transport::AudioReader::Source::setGain(float gain)
{
    mVolumeTargetValue = gain;
}

void Transport::AudioReader::Source::setStartPlayheadPosition(double position)
{
    auto const sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    auto const limit = static_cast<juce::int64>(mAudioFormatReader->lengthInSamples - 1);
    mStartPosition.store(std::min(static_cast<juce::int64>(std::round(position * sampleRate)), limit));
}

void Transport::AudioReader::Source::setLooping(bool shouldLoop)
{
    mIsLooping = shouldLoop;
}

void Transport::AudioReader::Source::setLoopRange(juce::Range<double> const& loopRange)
{
    auto const sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    auto const start = static_cast<juce::int64>(std::round(loopRange.getStart() * sampleRate));
    auto const end = static_cast<juce::int64>(std::round(loopRange.getEnd() * sampleRate));
    mLoopRange.store({start, end});
}

void Transport::AudioReader::Source::setStopAtLoopEnd(bool shouldStop)
{
    mStopAtLoopEnd = shouldStop;
}

Transport::AudioReader::ResamplingSource::ResamplingSource(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, int numChannels)
: mSource(std::move(audioFormatReader))
, mResamplingAudioSource(&mSource, false, numChannels)
{
}

void Transport::AudioReader::ResamplingSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlWeakAssert(sampleRate > 0.0);
    sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    auto sourceSampleRate = mSource.getSampleRate();
    anlWeakAssert(sourceSampleRate > 0.0);
    sourceSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;

    mResamplingAudioSource.setResamplingRatio(sourceSampleRate / sampleRate);
    mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Transport::AudioReader::ResamplingSource::releaseResources()
{
    mResamplingAudioSource.releaseResources();
}

void Transport::AudioReader::ResamplingSource::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mResamplingAudioSource.getNextAudioBlock(bufferToFill);
}

bool Transport::AudioReader::ResamplingSource::isPlaying() const
{
    return mSource.isPlaying();
}

double Transport::AudioReader::ResamplingSource::getReadPlayheadPosition() const
{
    return mSource.getReadPlayheadPosition();
}

void Transport::AudioReader::ResamplingSource::setPlaying(bool shouldPlay)
{
    mSource.setPlaying(shouldPlay);
}

void Transport::AudioReader::ResamplingSource::setGain(float gain)
{
    mSource.setGain(gain);
}

void Transport::AudioReader::ResamplingSource::setStartPlayheadPosition(double position)
{
    mSource.setStartPlayheadPosition(position);
}

void Transport::AudioReader::ResamplingSource::setLooping(bool shouldLoop)
{
    mSource.setLooping(shouldLoop);
}

void Transport::AudioReader::ResamplingSource::setLoopRange(juce::Range<double> const& loopRange)
{
    mSource.setLoopRange(loopRange);
}

void Transport::AudioReader::ResamplingSource::setStopAtLoopEnd(bool shouldStop)
{
    mSource.setStopAtLoopEnd(shouldStop);
}

Transport::AudioReader::AudioReader(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::playback:
            {
                if(static_cast<bool>(acsr.getAttr<AttrType::playback>()))
                {
                    auto instance = mSourceManager.getInstance();
                    if(instance != nullptr)
                    {
                        instance->setPlaying(true);
                    }
                    startTimer(20);
                }
                else
                {
                    stopTimer();
                    auto instance = mSourceManager.getInstance();
                    if(instance != nullptr)
                    {
                        instance->setPlaying(false);
                    }
                }
            }
            break;
            case AttrType::startPlayhead:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setStartPlayheadPosition(acsr.getAttr<AttrType::startPlayhead>());
                }
            }
            break;
            case AttrType::runningPlayhead:
                break;
            case AttrType::looping:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setLooping(acsr.getAttr<AttrType::looping>());
                }
            }
            break;
            case AttrType::loopRange:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setLoopRange(acsr.getAttr<AttrType::loopRange>());
                }
            }
            break;
            case AttrType::stopAtLoopEnd:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setStopAtLoopEnd(acsr.getAttr<AttrType::stopAtLoopEnd>());
                }
            }
            break;
            case AttrType::gain:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setGain(static_cast<float>(acsr.getAttr<AttrType::gain>()));
                }
            }
            break;
            case AttrType::markers:
            case AttrType::magnetize:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Transport::AudioReader::~AudioReader()
{
    mAccessor.removeListener(mListener);
}

void Transport::AudioReader::setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
{
    if(audioFormatReader == nullptr)
    {
        mSourceManager.setInstance(nullptr);
        return;
    }
    auto source = std::make_shared<ResamplingSource>(std::move(audioFormatReader), static_cast<int>(audioFormatReader->numChannels));
    if(source != nullptr)
    {
        source->setGain(static_cast<float>(mAccessor.getAttr<AttrType::gain>()));
        source->setStartPlayheadPosition(mAccessor.getAttr<AttrType::startPlayhead>());
        source->setLooping(mAccessor.getAttr<AttrType::looping>());
        source->setLoopRange(mAccessor.getAttr<AttrType::loopRange>());
        source->setStopAtLoopEnd(mAccessor.getAttr<AttrType::stopAtLoopEnd>());
        source->prepareToPlay(mSamplesPerBlockExpected, mSampleRate);
        source->setPlaying(mAccessor.getAttr<AttrType::playback>());
    }
    mSourceManager.setInstance(source);
}

void Transport::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mSamplesPerBlockExpected = samplesPerBlockExpected;
    mSampleRate = sampleRate;
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->setGain(static_cast<float>(mAccessor.getAttr<AttrType::gain>()));
        instance->setLooping(mAccessor.getAttr<AttrType::looping>());
        instance->setPlaying(mAccessor.getAttr<AttrType::playback>());
        instance->setStopAtLoopEnd(mAccessor.getAttr<AttrType::stopAtLoopEnd>());
        instance->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
}

void Transport::AudioReader::releaseResources()
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->releaseResources();
    }
}

void Transport::AudioReader::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->getNextAudioBlock(bufferToFill);
    }
    else
    {
        bufferToFill.buffer->clear();
    }
}

void Transport::AudioReader::timerCallback()
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        mAccessor.setAttr<AttrType::runningPlayhead>(instance->getReadPlayheadPosition(), NotificationType::synchronous);
        mAccessor.setAttr<AttrType::playback>(instance->isPlaying(), NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
