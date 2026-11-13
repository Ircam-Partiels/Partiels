#include "AnlDocumentAudioReader.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

std::tuple<std::unique_ptr<juce::AudioFormatReader>, juce::StringArray> Document::createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager)
{
    using ReaderLayout = std::tuple<std::unique_ptr<juce::AudioFormatReader>, int>;
    using ChannelLayout = AudioFileLayout::ChannelLayout;

    class AudioFormatReader
    : public juce::AudioFormatReader
    {
    public:
        AudioFormatReader(std::vector<ReaderLayout> channels)
        : juce::AudioFormatReader(nullptr, "AnlAudioReader")
        , mChannels(std::move(channels))
        {
            int maxChannels = 0;
            hasMultipleSampleRate = false;
            numChannels = 0u;
            usesFloatingPointData = true;
            for(auto const& channel : mChannels)
            {
                auto const& reader = std::get<0>(channel);
                if(reader != nullptr)
                {
                    sampleRate = sampleRate > 0.0 ? sampleRate : reader->sampleRate;
                    hasMultipleSampleRate = std::abs(sampleRate - reader->sampleRate) > std::numeric_limits<double>::epsilon() ? true : hasMultipleSampleRate;
                    bitsPerSample = std::max(reader->bitsPerSample, bitsPerSample);
                    lengthInSamples = std::max(reader->lengthInSamples, lengthInSamples);
                    maxChannels = std::max(static_cast<int>(reader->numChannels), maxChannels);
                    numChannels += std::get<1>(channel) == ChannelLayout::all ? reader->numChannels : 1u;
                }
                else
                {
                    numChannels += 1u;
                }
            }
            MiscWeakAssert(mChannels.empty() || maxChannels > 0);
            mBuffer.setSize(std::max(maxChannels, 1), 8192);
        }

        ~AudioFormatReader() override = default;

        bool readSamples(int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
        {
            auto constexpr scaleFactor = 1.0f / static_cast<float>(0x7fffffff);

            int channelIndex = 0;
            while(channelIndex < numDestChannels)
            {
                auto& channelInfo = mChannels[static_cast<size_t>(channelIndex)];
                auto& reader = std::get<0>(channelInfo);
                auto const layout = std::get<1>(channelInfo);

                if(destChannels[channelIndex] != nullptr && reader != nullptr)
                {
                    auto const usedNumChannels = layout < 0 ? static_cast<int>(reader->numChannels) : layout + 1;
                    auto startSample = startSampleInFile;
                    auto remaininSamples = numSamples;
                    auto outputOffset = startOffsetInDestBuffer;

                    while(remaininSamples > 0)
                    {
                        mBuffer.clear();
                        auto* const* internalBuffer = mBuffer.getArrayOfWritePointers();
                        auto const bufferSize = std::min(remaininSamples, mBuffer.getNumSamples());
                        if(!reader->readSamples(reinterpret_cast<int* const*>(internalBuffer), usedNumChannels, 0, startSample, bufferSize))
                        {
                            return false;
                        }
                        if(layout >= 0) // one channel
                        {
                            if(!reader->usesFloatingPointData)
                            {
                                juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[static_cast<size_t>(layout)], reinterpret_cast<int*>(internalBuffer[static_cast<size_t>(layout)]), scaleFactor, bufferSize);
                            }
                            auto* outputBuffer = reinterpret_cast<float*>(destChannels[channelIndex] + outputOffset);
                            juce::FloatVectorOperations::copy(outputBuffer, internalBuffer[static_cast<size_t>(layout)], bufferSize);
                        }
                        else if(layout == ChannelLayout::mono)
                        {
                            if(!reader->usesFloatingPointData)
                            {
                                juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[0_z], reinterpret_cast<int*>(internalBuffer[0_z]), scaleFactor, bufferSize);
                            }
                            auto* outputBuffer = reinterpret_cast<float*>(destChannels[channelIndex] + outputOffset);
                            juce::FloatVectorOperations::copy(outputBuffer, internalBuffer[0_z], bufferSize);
                            for(size_t channelToCopy = 1_z; channelToCopy < static_cast<size_t>(usedNumChannels); ++channelToCopy)
                            {
                                if(!reader->usesFloatingPointData)
                                {
                                    juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[channelToCopy], reinterpret_cast<int*>(internalBuffer[channelToCopy]), scaleFactor, bufferSize);
                                }
                                juce::FloatVectorOperations::add(outputBuffer, internalBuffer[channelToCopy], bufferSize);
                            }
                            juce::FloatVectorOperations::multiply(outputBuffer, 1.0f / static_cast<float>(usedNumChannels), bufferSize);
                        }
                        else if(layout == ChannelLayout::all)
                        {
                            for(size_t channelToCopy = 0_z; channelToCopy < static_cast<size_t>(usedNumChannels); ++channelToCopy)
                            {
                                if(!reader->usesFloatingPointData)
                                {
                                    juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[channelToCopy], reinterpret_cast<int*>(internalBuffer[channelToCopy]), scaleFactor, bufferSize);
                                }
                                auto* outputBuffer = reinterpret_cast<float*>(destChannels[static_cast<size_t>(channelIndex) + channelToCopy] + outputOffset);
                                juce::FloatVectorOperations::copy(outputBuffer, internalBuffer[channelToCopy], bufferSize);
                            }
                        }
                        else
                        {
                            MiscWeakAssert(false && "unsupported layout");
                            return false;
                        }

                        startSample += static_cast<juce::int64>(bufferSize);
                        outputOffset += bufferSize;
                        remaininSamples -= bufferSize;
                    }
                    channelIndex += layout == ChannelLayout::all ? usedNumChannels : 1;
                }
                else
                {
                    ++channelIndex;
                }
            }
            return true;
        }

        bool hasMultipleSampleRate = false;

    private:
        std::vector<ReaderLayout> mChannels;
        juce::AudioBuffer<float> mBuffer;
    };

    juce::StringArray errors;
    auto const audioReaderLayout = accessor.getAttr<AttrType::reader>();
    std::vector<ReaderLayout> readers;
    for(size_t i = 0; i < audioReaderLayout.size(); ++i)
    {
        auto const errorPrepend = juce::translate("Channel CHINDEX:").replace("CHINDEX", juce::String(i + 1));
        auto const& file = audioReaderLayout[i].file;
        if(!file.existsAsFile())
        {
            errors.add(errorPrepend + " " + juce::translate("The file FILENAME doesn't exist.").replace("FILENAME", file.getFullPathName()));
        }
        else
        {
            auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(file));
            if(audioFormatReader == nullptr)
            {
                errors.add(errorPrepend + " " + juce::translate("The input stream cannot be created for the file FILENAME.").replace("FILENAME", file.getFullPathName()));
            }
            else
            {
                readers.emplace_back(std::move(audioFormatReader), audioReaderLayout[i].channel);
            }
        }
    }

    auto reader = std::make_unique<AudioFormatReader>(std::move(readers));
    if(reader->hasMultipleSampleRate)
    {
        errors.add(juce::translate("The sample rates of the audio files are not identical!"));
    }
    else if(reader->sampleRate < std::numeric_limits<double>::epsilon())
    {
        return std::make_tuple(nullptr, errors);
    }
    return std::make_tuple(std::move(reader), errors);
}

namespace Document
{
    //! @brief Gets the number of channels of the audio format reader that each audio file layout provides
    //! @details The audio files that cannot be read provide no channel (see createAudioFormatReader).
    static std::vector<size_t> getLayoutNumChannels(std::vector<AudioFileLayout> const& layouts, juce::AudioFormatManager& audioFormatManager)
    {
        std::vector<size_t> numChannels;
        numChannels.reserve(layouts.size());
        for(auto const& layout : layouts)
        {
            auto const audioFormatReader = layout.file.existsAsFile() ? std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(layout.file)) : nullptr;
            if(audioFormatReader == nullptr)
            {
                numChannels.push_back(0_z);
            }
            else
            {
                numChannels.push_back(layout.channel == AudioFileLayout::ChannelLayout::all ? static_cast<size_t>(audioFormatReader->numChannels) : 1_z);
            }
        }
        return numChannels;
    }
} // namespace Document

Document::AudioReader::AudioReader(Accessor& accessor, juce::AudioFormatManager& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mTransportAudioReader(mAccessor.getAcsr<AcsrType::transport>())
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const files = acsr.getAttr<AttrType::reader>();
                mLayoutNumChannels = getLayoutNumChannels(files, mAudioFormatManager);
                updateChannelGains();
                if(files.empty())
                {
                    mTransportAudioReader.setAudioFormatReader(nullptr);
                    return;
                }
                mTransportAudioReader.setAudioFormatReader(std::get<0>(createAudioFormatReader(mAccessor, mAudioFormatManager)));
            }
            break;
            case AttrType::mutedChannels:
            case AttrType::soloedChannels:
            {
                updateChannelGains();
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
            case AttrType::description:
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
    auto* buffer = bufferToFill.buffer;
    if(buffer == nullptr)
    {
        return;
    }
    std::unique_lock<audio_spin_mutex> lock(mGainsMutex, std::try_to_lock);
    if(!lock.owns_lock())
    {
        // The gains are being updated so the state of the channels is unknown, silence is safer
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    MiscWeakAssert(mTargetGains.size() == mCurrentGains.size());
    auto const numChannels = std::min({static_cast<size_t>(buffer->getNumChannels()), mTargetGains.size(), mCurrentGains.size()});
    for(auto channel = 0_z; channel < numChannels; ++channel)
    {
        // The gain is ramped over the block to avoid clicks when a channel is muted or unmuted
        buffer->applyGainRamp(static_cast<int>(channel), bufferToFill.startSample, bufferToFill.numSamples, mCurrentGains[channel], mTargetGains[channel]);
        mCurrentGains[channel] = mTargetGains[channel];
    }
}

void Document::AudioReader::updateChannelGains()
{
    std::vector<float> targetGains;
    for(auto index = 0_z; index < mLayoutNumChannels.size(); ++index)
    {
        auto const gain = Tools::isChannelAudible(mAccessor, index) ? 1.0f : 0.0f;
        targetGains.insert(targetGains.end(), mLayoutNumChannels[index], gain);
    }
    // The vectors are allocated before locking so the audio thread is never blocked by an allocation
    auto currentGains = targetGains;
    std::unique_lock<audio_spin_mutex> lock(mGainsMutex);
    if(mCurrentGains.size() != targetGains.size())
    {
        // The audio format reader changed so there is no previous gain to ramp from
        std::swap(mCurrentGains, currentGains);
    }
    std::swap(mTargetGains, targetGains);
    lock.unlock();
}

ANALYSE_FILE_END
