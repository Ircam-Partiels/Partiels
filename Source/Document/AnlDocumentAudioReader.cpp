#include "AnlDocumentAudioReader.h"

ANALYSE_FILE_BEGIN

std::unique_ptr<juce::AudioFormatReader> Document::createAudioFormatReader(Accessor const& accessor, juce::AudioFormatManager& audioFormatManager, AlertType alertType)
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
            numChannels = static_cast<unsigned int>(mChannels.size());
            usesFloatingPointData = true;
            for(auto const& channel : mChannels)
            {
                auto const& reader = std::get<0>(channel);
                if(reader != nullptr)
                {
                    sampleRate = std::max(reader->sampleRate, sampleRate);
                    bitsPerSample = std::max(reader->bitsPerSample, bitsPerSample);
                    lengthInSamples = std::max(reader->lengthInSamples, lengthInSamples);
                    maxChannels = static_cast<int>(std::max(reader->numChannels, numChannels));
                }
            }
            mBuffer.setSize(maxChannels, 8192);
        }

        ~AudioFormatReader() override = default;

        bool readSamples(int** destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
        {
            constexpr auto scaleFactor = 1.0f / static_cast<float>(0x7fffffff);

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
                        auto result = reader->readSamples(reinterpret_cast<int**>(internalBuffer), numChannels, 0, startSampleInFile, numSamples);
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
                            while(channelToCopy >= 0)
                            {
                                if(!reader->usesFloatingPointData)
                                {
                                    auto* internalBufferChannel = internalBuffer[static_cast<size_t>(channelToCopy)];
                                    juce::FloatVectorOperations::convertFixedToFloat(internalBufferChannel, reinterpret_cast<int*>(internalBufferChannel), scaleFactor, bufferSize);
                                }
                                juce::FloatVectorOperations::add(outputBuffer, internalBuffer[static_cast<size_t>(channelToCopy)], bufferSize);
                                --channelToCopy;
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

    private:
        std::vector<ReaderLayout> mChannels;
        juce::AudioBuffer<float> mBuffer;
    };
    using AlertIconType = juce::AlertWindow::AlertIconType;
    auto const errorMessage = juce::translate("Audio format reader cannot be loaded!");

    auto const audioReaderLayout = accessor.getAttr<AttrType::reader>();
    if(audioReaderLayout.empty())
    {
        return nullptr;
    }

    std::vector<ReaderLayout> readers;
    for(auto const& audioReaderChannel : audioReaderLayout)
    {
        auto audioFormatReader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(audioReaderChannel.file));
        if(audioFormatReader == nullptr)
        {
            if(alertType == AlertType::window)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The audio format reader cannot be allocated for the input stream FLNAME.").replace("FLNAME", audioReaderChannel.file.getFullPathName()));
            }
            return nullptr;
        }
        readers.emplace_back(std::move(audioFormatReader), audioReaderChannel.channel);
    }

    return std::make_unique<AudioFormatReader>(std::move(readers));
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
                mTransportAudioReader.setAudioFormatReader(createAudioFormatReader(mAccessor, mAudioFormatManager, AlertType::window));
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
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
