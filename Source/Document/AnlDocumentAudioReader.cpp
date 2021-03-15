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
    anlStrongAssert(std::abs(mAudioFormatReader->sampleRate - sampleRate) < std::numeric_limits<double>::epsilon());
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
    mVolume.setTargetValue(mVolumeTargetValue.load());
    mAudioFormatReaderSource.getNextAudioBlock(bufferToFill);
    for(auto i = 0; i < bufferToFill.numSamples; i++)
    {
        bufferToFill.buffer->applyGain(bufferToFill.startSample + i, 1, mVolume.getNextValue());
    }
}

void Document::AudioReader::Source::setNextReadPosition(juce::int64 newPosition)
{
    mAudioFormatReaderSource.setNextReadPosition(newPosition);
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

void Document::AudioReader::Source::setGain(float gain)
{
    mVolumeTargetValue = gain;
}

Document::AudioReader::AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        auto getLastPosition = [&]() -> juce::int64
        {
            auto instance = mSourceManager.getInstance();
            if(instance != nullptr)
            {
                return instance->getTotalLength();
            }
            return 0;
        };
        
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
                auto source = std::make_shared<Source>(std::move(audioFormatReader));
                if(source != nullptr)
                {
                    source->setGain(static_cast<float>(acsr.getAttr<AttrType::gain>()));
                    source->prepareToPlay(mSamplesPerBlockExpected, mSampleRate);
                }
                mSourceManager.setInstance(source);
            }
                break;
            case AttrType::isLooping:
            {
                mIsLooping.store(acsr.getAttr<AttrType::isLooping>());
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
            case AttrType::playheadPosition:
            {
                if(!mIsPlaying.load())
                {
                    auto instance = mSourceManager.getInstance();
                    if(instance != nullptr)
                    {
                        auto const sampleRate = mSampleRate > 0.0 ? mSampleRate : 44100.0;
                        auto const position = acsr.getAttr<AttrType::playheadPosition>();
                        instance->setNextReadPosition(static_cast<juce::int64>(position * sampleRate));
                    }
                }
            }
                break;
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

double Document::AudioReader::getSampleRate() const
{
    return mSampleRate;
}

void Document::AudioReader::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mSamplesPerBlockExpected = samplesPerBlockExpected;
    mSampleRate = sampleRate;
    auto instance = mSourceManager.getInstance();
    if(instance != nullptr)
    {
        instance->setGain(static_cast<float>(mAccessor.getAttr<AttrType::gain>()));
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
            auto const readPosition = instance->getNextReadPosition();
            auto const endPosition = instance->getTotalLength();
            if(readPosition >= endPosition)
            {
                mReadPosition = 0;
                instance->setNextReadPosition(0);
                mIsPlaying.store(mIsLooping);
            }
            else
            {
                mReadPosition = readPosition;
            }
        }
    }
    else
    {
        bufferToFill.buffer->clear();
    }
}

void Document::AudioReader::timerCallback()
{
    auto const sampleRate = mSampleRate > 0.0 ? mSampleRate : 44100.0;
    mAccessor.setAttr<AttrType::playheadPosition>(static_cast<double>(mReadPosition.load()) / sampleRate, NotificationType::synchronous);
    mAccessor.setAttr<AttrType::isPlaybackStarted>(mIsPlaying.load(), NotificationType::synchronous);
}

ANALYSE_FILE_END
