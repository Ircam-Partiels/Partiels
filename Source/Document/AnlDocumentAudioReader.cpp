#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

std::tuple<std::unique_ptr<juce::AudioFormatReader>, juce::StringArray> Document::createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager)
{
    using ReaderLayout = std::tuple<std::unique_ptr<juce::AudioFormatReader>, int>;

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
                    numChannels += std::get<1>(channel) == -2 ? reader->numChannels : 1u;
                }
                else
                {
                    numChannels += 1u;
                }
            }
            anlWeakAssert(mChannels.empty() || maxChannels > 0);
            mBuffer.setSize(std::max(maxChannels, 1), 8192);
        }

        ~AudioFormatReader() override = default;

        bool readSamples(int** destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
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
                    auto const numChannels = layout < 0 ? static_cast<int>(reader->numChannels) : layout + 1;
                    auto startSample = startSampleInFile;
                    auto remaininSamples = numSamples;
                    auto outputOffset = startOffsetInDestBuffer;

                    while(remaininSamples > 0)
                    {
                        mBuffer.clear();
                        auto** internalBuffer = mBuffer.getArrayOfWritePointers();
                        auto const bufferSize = std::min(remaininSamples, mBuffer.getNumSamples());
                        auto result = reader->readSamples(reinterpret_cast<int**>(internalBuffer), numChannels, 0, startSampleInFile, bufferSize);
                        if(!result)
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
                        else if(layout == -1) // mono
                        {
                            if(!reader->usesFloatingPointData)
                            {
                                juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[0_z], reinterpret_cast<int*>(internalBuffer[0_z]), scaleFactor, bufferSize);
                            }
                            auto* outputBuffer = reinterpret_cast<float*>(destChannels[channelIndex] + outputOffset);
                            juce::FloatVectorOperations::copy(outputBuffer, internalBuffer[0_z], bufferSize);
                            for(size_t channelToCopy = 1_z; channelToCopy < static_cast<size_t>(numChannels); ++channelToCopy)
                            {
                                if(!reader->usesFloatingPointData)
                                {
                                    juce::FloatVectorOperations::convertFixedToFloat(internalBuffer[channelToCopy], reinterpret_cast<int*>(internalBuffer[channelToCopy]), scaleFactor, bufferSize);
                                }
                                juce::FloatVectorOperations::add(outputBuffer, internalBuffer[channelToCopy], bufferSize);
                            }
                            juce::FloatVectorOperations::multiply(outputBuffer, 1.0f / static_cast<float>(numChannels), bufferSize);
                        }
                        else if(layout == -2) // all
                        {
                            for(size_t channelToCopy = 0_z; channelToCopy < static_cast<size_t>(numChannels); ++channelToCopy)
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
                            anlWeakAssert(false && "unsupported layout");
                            return false;
                        }

                        startSample += static_cast<juce::int64>(bufferSize);
                        outputOffset += bufferSize;
                        remaininSamples -= bufferSize;
                    }
                    channelIndex += layout == -2 ? numChannels : 1;
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
    if(reader == nullptr)
    {
        errors.add(juce::translate("The audio reader cannot be allocated!"));
    }
    else if(reader->hasMultipleSampleRate)
    {
        errors.add(juce::translate("The sample rates of the audio files are not identical!"));
    }
    else if(reader->sampleRate < std::numeric_limits<double>::epsilon())
    {
        return std::make_tuple(nullptr, errors);
    }
    return std::make_tuple(std::move(reader), errors);
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
            case AttrType::reader:
            {
                auto const files = acsr.getAttr<AttrType::reader>();
                if(files.empty())
                {
                    mTransportAudioReader.setAudioFormatReader(nullptr);
                    return;
                }
                mTransportAudioReader.setAudioFormatReader(std::get<0>(createAudioFormatReader(mAccessor, mAudioFormatManager)));
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::grid:
            case AttrType::samplerate:
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
