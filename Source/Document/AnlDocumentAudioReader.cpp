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
            numChannels = static_cast<unsigned int>(mChannels.size());
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
                }
            }
            anlWeakAssert(maxChannels > 0);
            mBuffer.setSize(std::max(maxChannels, 1), 8192);
        }

        ~AudioFormatReader() override = default;

        bool readSamples(int** destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
        {
            auto constexpr scaleFactor = 1.0f / static_cast<float>(0x7fffffff);

            for(int channelIndex = 0; channelIndex < numDestChannels; ++channelIndex)
            {
                auto& channelInfo = mChannels[static_cast<size_t>(channelIndex)];
                auto& reader = std::get<0>(channelInfo);
                auto const layout = std::get<1>(channelInfo);
                if(destChannels[channelIndex] != nullptr && reader != nullptr)
                {
                    auto* outputBuffer = reinterpret_cast<float*>(destChannels[channelIndex] + startOffsetInDestBuffer);
                    auto const numChannels = layout < 0 ? static_cast<int>(reader->numChannels) : layout + 1;
                    auto startSample = startSampleInFile;
                    auto remaininSamples = numSamples;
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
                        auto channelToCopy = numChannels - 1;
                        if(!reader->usesFloatingPointData)
                        {
                            auto* internalBufferChannel = internalBuffer[static_cast<size_t>(channelToCopy)];
                            juce::FloatVectorOperations::convertFixedToFloat(internalBufferChannel, reinterpret_cast<int*>(internalBufferChannel), scaleFactor, bufferSize);
                        }
                        juce::FloatVectorOperations::copy(outputBuffer, internalBuffer[static_cast<size_t>(channelToCopy)], bufferSize);
                        if(layout < 0)
                        {
                            while(--channelToCopy >= 0)
                            {
                                if(!reader->usesFloatingPointData)
                                {
                                    auto* internalBufferChannel = internalBuffer[static_cast<size_t>(channelToCopy)];
                                    juce::FloatVectorOperations::convertFixedToFloat(internalBufferChannel, reinterpret_cast<int*>(internalBufferChannel), scaleFactor, bufferSize);
                                }
                                juce::FloatVectorOperations::add(outputBuffer, internalBuffer[static_cast<size_t>(channelToCopy)], bufferSize);
                            }
                            juce::FloatVectorOperations::multiply(outputBuffer, 1.0f / static_cast<float>(numChannels), bufferSize);
                        }
                        startSample += static_cast<juce::int64>(bufferSize);
                        outputBuffer += static_cast<size_t>(bufferSize);
                        remaininSamples -= bufferSize;
                    }
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
        auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(file));
        if(!file.existsAsFile())
        {
            errors.add(errorPrepend + " " + juce::translate("The file FILENAME doesn't exist.").replace("FILENAME", file.getFullPathName()));
        }
        else if(audioFormatReader == nullptr)
        {
            errors.add(errorPrepend + " " + juce::translate("The input stream cannot be created for the file FILENAME.").replace("FILENAME", file.getFullPathName()));
        }
        readers.emplace_back(std::move(audioFormatReader), audioReaderLayout[i].channel);
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
