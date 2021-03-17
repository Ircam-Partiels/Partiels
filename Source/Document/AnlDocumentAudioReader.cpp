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

Document::AudioReader::Source::Source(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mAudioFormatReader(std::move(audioFormatReader))
, mAudioFormatReaderSource(mAudioFormatReader.get(), false)
{
    anlStrongAssert(mAudioFormatReader != nullptr);
}

void Document::AudioReader::Source::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlStrongAssert(sampleRate > 0.0 && mAudioFormatReader->sampleRate > 0.0);
    anlStrongAssert(std::abs(mAudioFormatReader->sampleRate - sampleRate) < 0.001);
    sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    mAudioFormatReaderSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    mVolume.reset(mAudioFormatReader->sampleRate, static_cast<double>(samplesPerBlockExpected) / sampleRate);
    mVolume.setCurrentAndTargetValue(mVolumeTargetValue.load());
}

void Document::AudioReader::Source::releaseResources()
{
    mAudioFormatReaderSource.releaseResources();
}

void Document::AudioReader::Source::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    anlStrongAssert(buffer != nullptr);
    if(buffer == nullptr)
    {
        return;
    }
    
    mVolume.setTargetValue(mVolumeTargetValue.load());
    auto const endPosition = mAudioFormatReaderSource.getTotalLength();
    
    auto numSamplesToProceed = bufferToFill.numSamples;
    auto outputPosition = bufferToFill.startSample;
    while(numSamplesToProceed > 0)
    {
        if(mIsPlaying.load())
        {
            auto const currentReadPosition = mReadPosition.load();
            auto const numRemainingSamples = static_cast<int>(std::min(endPosition - currentReadPosition, static_cast<juce::int64>(numSamplesToProceed)));
            juce::AudioSourceChannelInfo tempBuffer(buffer, outputPosition, numRemainingSamples);
            
            mAudioFormatReaderSource.setNextReadPosition(currentReadPosition);
            mAudioFormatReaderSource.getNextAudioBlock(tempBuffer);
            auto const nextReadPosition = mAudioFormatReaderSource.getNextReadPosition();
            if(nextReadPosition >= endPosition)
            {
                mReadPosition = mStartPosition.load();
                mAudioFormatReaderSource.setNextReadPosition(mStartPosition.load());
                mIsPlaying.store(mIsPlaying.load() && mIsLooping.load());
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

double Document::AudioReader::Source::getSampleRate() const
{
    return mAudioFormatReader->sampleRate;
}

bool Document::AudioReader::Source::isPlaying() const
{
    return mIsPlaying.load();
}

double Document::AudioReader::Source::getReadPlayheadPosition() const
{
    auto const sampleRate = mAudioFormatReader->sampleRate > 0.0 ? mAudioFormatReader->sampleRate : 44100.0;
    return static_cast<double>(mReadPosition.load()) / sampleRate;
}

void Document::AudioReader::Source::setPlaying(bool shouldPlay)
{
    mReadPosition.store(mStartPosition.load());
    mIsPlaying = shouldPlay;
}

void Document::AudioReader::Source::setLooping(bool shouldLoop)
{
    mIsLooping = shouldLoop;
}

void Document::AudioReader::Source::setGain(float gain)
{
    mVolumeTargetValue = gain;
}

Document::AudioReader::ResamplingSource::ResamplingSource(std::unique_ptr<juce::AudioFormatReader> audioFormatReader)
: mSource(std::move(audioFormatReader))
{
}

void Document::AudioReader::ResamplingSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    anlWeakAssert(sampleRate > 0.0);
    sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    
    auto sourceSampleRate = mSource.getSampleRate();
    anlWeakAssert(sourceSampleRate > 0.0);
    sourceSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;
    
    mResamplingAudioSource.setResamplingRatio(sourceSampleRate / sampleRate);
    mResamplingAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void Document::AudioReader::ResamplingSource::releaseResources()
{
    mResamplingAudioSource.releaseResources();
}

void Document::AudioReader::ResamplingSource::getNextAudioBlock(juce::AudioSourceChannelInfo const& bufferToFill)
{
    mResamplingAudioSource.getNextAudioBlock(bufferToFill);
}

bool Document::AudioReader::ResamplingSource::isPlaying() const
{
    return mSource.isPlaying();
}

double Document::AudioReader::ResamplingSource::getReadPlayheadPosition() const
{
    return mSource.getReadPlayheadPosition();
}

void Document::AudioReader::ResamplingSource::setPlaying(bool shouldPlay)
{
    mSource.setPlaying(shouldPlay);
}

void Document::AudioReader::ResamplingSource::setLooping(bool shouldLoop)
{
    mSource.setLooping(shouldLoop);
}

void Document::AudioReader::ResamplingSource::setGain(float gain)
{
    mSource.setGain(gain);
}

Document::AudioReader::AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
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
                    mSourceManager.setInstance(nullptr);
                    return;
                }
                auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window);
                if(audioFormatReader == nullptr)
                {
                    mSourceManager.setInstance(nullptr);
                    return;
                }
                auto source = std::make_shared<ResamplingSource>(std::move(audioFormatReader));
                if(source != nullptr)
                {
                    source->setPlaying(acsr.getAttr<AttrType::isPlaybackStarted>());
                    source->setLooping(acsr.getAttr<AttrType::isLooping>());
                    source->setGain(static_cast<float>(acsr.getAttr<AttrType::gain>()));
                    source->prepareToPlay(mSamplesPerBlockExpected, mSampleRate);
                }
                mSourceManager.setInstance(source);
            }
                break;
            case AttrType::isLooping:
            {
                auto instance = mSourceManager.getInstance();
                if(instance != nullptr)
                {
                    instance->setLooping(acsr.getAttr<AttrType::isLooping>());
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
            case AttrType::isPlaybackStarted:
            {
                if(static_cast<bool>(acsr.getAttr<AttrType::isPlaybackStarted>()))
                {
                    auto instance = mSourceManager.getInstance();
                    if(instance != nullptr)
                    {
                        instance->setPlaying(true);
                    }
                    startTimer(50);
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
            case AttrType::playheadPosition:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::layout:
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
    mSamplesPerBlockExpected = samplesPerBlockExpected;
    mSampleRate = sampleRate;
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->setGain(static_cast<float>(mAccessor.getAttr<AttrType::gain>()));
        instance->setLooping(mAccessor.getAttr<AttrType::isLooping>());
        instance->setLooping(mAccessor.getAttr<AttrType::isPlaybackStarted>());
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
    else
    {
        bufferToFill.buffer->clear();
    }
}

void Document::AudioReader::timerCallback()
{
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        mAccessor.setAttr<AttrType::playheadPosition>(instance->getReadPlayheadPosition(), NotificationType::synchronous);
        mAccessor.setAttr<AttrType::isPlaybackStarted>(instance->isPlaying(), NotificationType::synchronous);
    }
}

ANALYSE_FILE_END
