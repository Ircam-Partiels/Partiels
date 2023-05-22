#include "AnlPluginProcessor.h"

ANALYSE_FILE_BEGIN

Plugin::Processor::LoadingError::LoadingError(char const* message)
: std::runtime_error(message)
{
}

Plugin::Processor::ParametersError::ParametersError(char const* message)
: std::runtime_error(message)
{
}

Plugin::Processor::CircularReader::CircularReader(juce::AudioFormatReader& audioFormatReader, size_t blockSize, size_t stepSize)
: mBlocksize(static_cast<int>(blockSize))
, mStepSize(static_cast<int>(stepSize))
, mAudioFormatReader(audioFormatReader)
, mBuffer(static_cast<int>(audioFormatReader.numChannels), mBlocksize * 2)
, mReaderPosition(-static_cast<juce::int64>(std::round(static_cast<double>(blockSize) / 2.0)))
{
    mOutputBuffer.resize(static_cast<size_t>(audioFormatReader.numChannels));
}

juce::int64 Plugin::Processor::CircularReader::getLengthInSamples() const
{
    return mAudioFormatReader.lengthInSamples;
}

double Plugin::Processor::CircularReader::getSampleRate() const
{
    return mAudioFormatReader.sampleRate;
}

bool Plugin::Processor::CircularReader::hasReachedEnd() const
{
    anlWeakAssert(mPosition >= static_cast<juce::int64>(0));
    auto const offset = static_cast<juce::int64>(std::round(static_cast<double>(mBlocksize) / 2.0));
    anlWeakAssert(mPosition <= mAudioFormatReader.lengthInSamples + offset);
    return mPosition >= mAudioFormatReader.lengthInSamples + offset;
}

juce::int64 Plugin::Processor::CircularReader::getPosition() const
{
    return mPosition;
}

float const** Plugin::Processor::CircularReader::getNextBlock()
{
    auto* const* inputPointers = mBuffer.getArrayOfWritePointers();
    auto const numChannels = mBuffer.getNumChannels();

    auto const offset = static_cast<juce::int64>(std::round(static_cast<double>(mBlocksize) / 2.0));
    auto const maxLength = mAudioFormatReader.lengthInSamples + offset;
    mPosition = std::min(mPosition + static_cast<juce::int64>(mStepSize), maxLength);

    if(mStepSize >= mBlocksize)
    {
        juce::AudioBuffer<float> input(inputPointers, numChannels, 0, mBlocksize);
        mAudioFormatReader.read(input.getArrayOfWritePointers(), numChannels, mReaderPosition, mBlocksize);

        mReaderPosition += static_cast<juce::int64>(mStepSize);

        for(size_t channel = 0; channel < static_cast<size_t>(numChannels); ++channel)
        {
            mOutputBuffer[channel] = inputPointers[channel];
        }
        return mOutputBuffer.data();
    }

    // Expected number of samples to read (equivalent to the step except for the first call if the block size is greater)
    auto const bufferSize = std::max(static_cast<int>(static_cast<juce::int64>(mBlocksize) - (mReaderPosition + offset)), mStepSize);
    // The number of sample that can be used from the previous block
    auto const hopeSize = std::max(mBlocksize - bufferSize, 0);

    auto bufferPosition = mBufferPosition;
    if(bufferPosition + mStepSize > mBuffer.getNumSamples())
    {
        auto const bufferSource = bufferPosition - hopeSize;
        anlStrongAssert(bufferSource != bufferPosition);
        if(bufferSource != bufferPosition)
        {
            for(int channel = 0; channel < numChannels; ++channel)
            {
                mBuffer.copyFrom(channel, 0, mBuffer.getReadPointer(channel, bufferSource), hopeSize);
            }
        }
        mBuffer.clear(hopeSize, mBuffer.getNumSamples() - hopeSize);
        bufferPosition = hopeSize;
        anlStrongAssert(bufferSize == static_cast<juce::int64>(mStepSize));
    }

    juce::AudioBuffer<float> input(inputPointers, numChannels, bufferPosition, bufferSize);
    auto const silence = static_cast<int>(std::max(mReaderPosition + bufferSize - maxLength, static_cast<juce::int64>(0)));
    if(silence >= bufferSize)
    {
        input.clear();
    }
    else
    {
        mAudioFormatReader.read(input.getArrayOfWritePointers(), numChannels, mReaderPosition, bufferSize - silence);
        input.clear(bufferSize - silence, silence);
    }

    mBufferPosition = bufferPosition + bufferSize;
    mReaderPosition += static_cast<juce::int64>(bufferSize);

    auto const outputBufferPosition = std::max(bufferPosition - hopeSize, 0);
    for(size_t channel = 0; channel < static_cast<size_t>(numChannels); ++channel)
    {
        mOutputBuffer[channel] = inputPointers[channel] + outputBufferPosition;
    }
    return mOutputBuffer.data();
}

Plugin::Processor::Processor(juce::AudioFormatReader& audioFormatReader, std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins, size_t const feature, State const& state)
: mPlugins(std::move(plugins))
, mCircularReader(audioFormatReader, state.blockSize, state.stepSize)
, mFeature(feature)
, mState(state)
{
}

bool Plugin::Processor::prepareToAnalyze(std::vector<std::vector<Result>>& results)
{
    anlStrongAssert(!mPlugins.empty());
    if(mPlugins.empty() || std::any_of(mPlugins.cbegin(), mPlugins.cend(), [](auto const& plugin)
                                       {
                                           return plugin == nullptr;
                                       }))
    {
        return false;
    }

    auto const blockSize = mState.blockSize;
    auto const stepSize = mState.stepSize;
    anlStrongAssert(blockSize > 0 && stepSize > 0);
    if(blockSize <= 0 || stepSize <= 0)
    {
        return false;
    }

    auto const descriptors = mPlugins.at(0)->getOutputDescriptors();
    anlStrongAssert(mFeature < descriptors.size());
    if(mFeature >= descriptors.size())
    {
        return false;
    }

    results.resize(mPlugins.size());
    auto const& descriptor = descriptors[mFeature];
    if(descriptor.sampleType == Output::SampleType::OneSamplePerStep)
    {
        auto const length = static_cast<double>(mCircularReader.getLengthInSamples());
        auto const size = static_cast<size_t>(std::ceil(length / static_cast<double>(mState.stepSize)));
        for(auto& channelResults : results)
        {
            channelResults.reserve(size);
        }
    }
    else if(descriptor.sampleType == Output::SampleType::FixedSampleRate)
    {
        auto const length = static_cast<double>(mCircularReader.getLengthInSamples());
        auto const duration = length / mCircularReader.getSampleRate();
        auto const size = static_cast<size_t>(std::ceil(duration * descriptor.sampleRate));
        for(auto& channelResults : results)
        {
            channelResults.reserve(size);
        }
    }
    return true;
}

bool Plugin::Processor::setPrecomputingResults(std::vector<std::vector<Result>> const& results)
{
    anlWeakAssert(!mPlugins.empty());
    anlWeakAssert(results.empty() || results.size() == 1_z || results.size() == mPlugins.size());
    Vamp::Plugin::FeatureSet fs;
    if(results.size() == 1_z)
    {
        fs[static_cast<int>(mFeature)] = results.at(0_z);
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            mPlugins[index]->setPreComputingFeatures(fs);
        }
    }
    else if(results.size() == mPlugins.size())
    {
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            fs[static_cast<int>(mFeature)] = results.at(index);
            mPlugins[index]->setPreComputingFeatures(fs);
        }
    }
    else
    {
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            mPlugins[index]->setPreComputingFeatures(fs);
        }
    }
    return results.empty() || results.size() == 1_z || results.size() == mPlugins.size();
}

bool Plugin::Processor::performNextAudioBlock(std::vector<std::vector<Result>>& results)
{
    anlStrongAssert(!mPlugins.empty());
    if(mPlugins.empty() || std::any_of(mPlugins.cbegin(), mPlugins.cend(), [](auto const& plugin)
                                       {
                                           return plugin == nullptr;
                                       }))
    {
        return false;
    }

    auto const feature = mFeature;
    auto const blockSize = mState.blockSize;
    auto const stepSize = mState.stepSize;
    anlStrongAssert(blockSize > 0 && stepSize > 0);
    if(blockSize <= 0 || stepSize <= 0)
    {
        return false;
    }

    auto const position = mCircularReader.getPosition();
    auto const sampleRate = mCircularReader.getSampleRate();
    auto const rt = Vamp::RealTime::frame2RealTime(static_cast<long>(position), static_cast<unsigned int>(sampleRate));

    if(mCircularReader.hasReachedEnd())
    {
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            auto result = mPlugins[index]->getRemainingFeatures();
            auto it = result.find(static_cast<int>(feature));
            if(it != result.end())
            {
                for(auto& that : it->second)
                {
                    if(!that.hasTimestamp)
                    {
                        that.hasTimestamp = true;
                        that.timestamp = rt;
                    }
                    results[index].emplace_back(std::move(that));
                }
            }
        }
        return false;
    }

    auto const** block = mCircularReader.getNextBlock();
    for(size_t index = 0; index < mPlugins.size(); ++index)
    {
        auto result = mPlugins[index]->process(block, rt);
        auto it = result.find(static_cast<int>(feature));
        if(it != result.end())
        {
            for(auto& that : it->second)
            {
                if(!that.hasTimestamp)
                {
                    that.hasTimestamp = true;
                    that.timestamp = rt;
                }
                results[index].emplace_back(std::move(that));
            }
        }
        block += mPlugins[index]->getMaxChannelCount();
    }
    return true;
}

float Plugin::Processor::getAdvancement() const
{
    auto const position = static_cast<float>(mCircularReader.getPosition());
    auto const length = static_cast<float>(mCircularReader.getLengthInSamples()) + std::round(static_cast<float>(mState.blockSize) / 2.0f);
    return position / length;
}

Plugin::Input Plugin::Processor::getInput() const
{
    anlStrongAssert(!mPlugins.empty());
    if(mPlugins.empty() || mPlugins.at(0) == nullptr)
    {
        return {};
    }

    auto const output = getOutput();
    auto const descriptors = mPlugins.at(0)->getInputDescriptors();
    auto const it = std::find_if(descriptors.cbegin(), descriptors.cend(), [&](auto const& descriptor)
                                 {
                                     return descriptor.identifier == output.identifier;
                                 });
    return it != descriptors.cend() ? *it : Plugin::Input{};
}

Plugin::Output Plugin::Processor::getOutput() const
{
    anlStrongAssert(!mPlugins.empty());
    if(mPlugins.empty() || mPlugins.at(0) == nullptr)
    {
        return {};
    }

    auto const descriptors = mPlugins.at(0)->getOutputDescriptors();
    anlStrongAssert(descriptors.size() > mFeature);
    return descriptors.size() > mFeature ? descriptors[mFeature] : Plugin::Output{};
}

std::unique_ptr<Plugin::Processor> Plugin::Processor::create(Key const& key, State const& state, juce::AudioFormatReader& audioFormatReader)
{
    using namespace Vamp::HostExt;
    auto* pluginLoader = PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    anlStrongAssert(!key.identifier.empty());
    if(key.identifier.empty())
    {
        throw LoadingError("plugin key is not defined.");
    }
    anlStrongAssert(!key.feature.empty());
    if(key.feature.empty())
    {
        throw LoadingError("plugin feature is not defined.");
    }
    anlStrongAssert(state.blockSize > 0);
    if(state.blockSize == 0)
    {
        throw ParametersError("plugin block size is invalid");
    }
    anlStrongAssert(state.stepSize > 0);
    if(state.stepSize == 0)
    {
        throw ParametersError("plugin step size is invalid");
    }

    std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins;
    auto addAndInitializeInstance = [&](std::unique_ptr<Ive::PluginWrapper>& plugin, size_t numChannels) -> bool
    {
        MiscWeakAssert(plugin != nullptr);
        if(plugin == nullptr)
        {
            return false;
        }
        if(auto* adapter = plugin->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
        {
            adapter->setWindowType(state.windowType);
        }

        auto const descriptors = plugin->getParameterDescriptors();
        for(auto const& parameter : state.parameters)
        {
            if(std::any_of(descriptors.cbegin(), descriptors.cend(), [&](auto const& descriptor)
                           {
                               return descriptor.identifier == parameter.first;
                           }))
            {
                plugin->setParameter(parameter.first, parameter.second);
            }
            else
            {
                throw ParametersError("plugin parameter is invalid");
            }
        }

        if(!plugin->initialise(numChannels, state.stepSize, state.blockSize))
        {
            throw ParametersError("plugin initialization failed");
        }
        plugins.push_back(std::move(plugin));
        return true;
    };

    auto const sampleRate = static_cast<float>(audioFormatReader.sampleRate);
    auto instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
    if(instance == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }
    auto wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
    if(wrapper == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }

    auto const outputs = wrapper->getOutputDescriptors();
    auto const feature = std::find_if(outputs.cbegin(), outputs.cend(), [&](auto const& output)
                                      {
                                          return output.identifier == key.feature;
                                      });
    if(feature == outputs.cend())
    {
        throw LoadingError("plugin feature is invalid");
    }
    auto const featureIndex = static_cast<size_t>(std::distance(outputs.cbegin(), feature));

    auto const maxChannels = wrapper->getMaxChannelCount();
    auto numReaderChannels = static_cast<size_t>(audioFormatReader.numChannels);
    while(wrapper != nullptr)
    {
        auto const numChannels = std::min(maxChannels, numReaderChannels);
        numReaderChannels -= numChannels;
        if(!addAndInitializeInstance(wrapper, numChannels))
        {
            return nullptr;
        }
        if(numReaderChannels > 0_z)
        {
            instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
            if(instance == nullptr)
            {
                throw LoadingError("plugin library couldn't be loaded");
            }
            wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
            if(wrapper == nullptr)
            {
                throw LoadingError("plugin library couldn't be loaded");
            }
        }
    }

    auto processor = std::unique_ptr<Processor>(new Processor(audioFormatReader, std::move(plugins), featureIndex, state));
    if(processor == nullptr)
    {
        throw std::runtime_error("allocation failed");
    }
    return processor;
}

class Plugin::Processor::CircularReaderUnitTest
: public juce::UnitTest
{
public:
    CircularReaderUnitTest()
    : juce::UnitTest("CircularReader", "Plugin")
    {
    }

    ~CircularReaderUnitTest() override = default;

    void runTest() override
    {
        class Reader
        : public juce::AudioFormatReader
        {
        public:
            Reader(double sr, juce::int64 l)
            : juce::AudioFormatReader(nullptr, "InternalTest")
            {
                sampleRate = sr;
                bitsPerSample = sizeof(float);
                numChannels = 1;
                lengthInSamples = l;
                usesFloatingPointData = true;
            }

            ~Reader() override = default;

            bool readSamples(int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
            {
                for(int channelIndex = 0; channelIndex < numDestChannels; ++channelIndex)
                {
                    auto* channel = reinterpret_cast<float*>(destChannels[channelIndex]) + startOffsetInDestBuffer;
                    for(int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
                    {
                        auto const position = static_cast<juce::int64>(sampleIndex) + startSampleInFile;
                        auto const value = position < lengthInSamples ? static_cast<float>(position) : 0.0f;
                        channel[static_cast<size_t>(sampleIndex)] = value;
                    }
                }
                return true;
            }
        };

        Reader reader(44100.0, 20813);

        beginTest("check reader");
        {
            auto constexpr blockSize = 2048;
            juce::AudioBuffer<float> buffer(1, blockSize);
            for(juce::int64 index = 0; index < reader.lengthInSamples; index += blockSize)
            {
                reader.read(&buffer, 0, blockSize, index, true, false);
                for(int sampleIndex = 0; sampleIndex < blockSize; ++sampleIndex)
                {
                    auto const position = index + static_cast<juce::int64>(sampleIndex);
                    auto const value = position < reader.lengthInSamples ? static_cast<float>(position) : 0.0f;
                    expectWithinAbsoluteError(buffer.getSample(0, sampleIndex), value, 0.001f);
                }
            }
        }

        auto performTest = [&](size_t blockSize, size_t stepSize)
        {
            beginTest("check circular reader " + juce::String(blockSize) + " " + juce::String(stepSize));
            CircularReader circularReader(reader, blockSize, stepSize);
            float const** output = nullptr;
            juce::int64 position = static_cast<juce::int64>(0);
            auto const offset = static_cast<juce::int64>(std::round(static_cast<float>(blockSize) / 2.0f));
            while(!circularReader.hasReachedEnd())
            {
                position = circularReader.getPosition();
                output = circularReader.getNextBlock();
                for(size_t sampleIndex = 0_z; sampleIndex < blockSize; ++sampleIndex)
                {
                    auto const readerPosition = position + static_cast<juce::int64>(sampleIndex) - offset;
                    auto const expected = readerPosition >= static_cast<juce::int64>(0) && readerPosition < reader.lengthInSamples ? static_cast<float>(readerPosition) : 0.0f;
                    expectWithinAbsoluteError(output[0_z][sampleIndex], expected, 0.001f,
                                              "Position " + juce::String(position) + " " + juce::String(readerPosition));
                }
            }
            expectEquals(circularReader.getPosition() - offset, reader.lengthInSamples);
        };

        performTest(1024, 1024);
        performTest(1024, 2000);
        performTest(1024, 256);
        performTest(1024, 333);
    }
};

static Plugin::Processor::CircularReaderUnitTest pluginProcessorCircularReaderUnitTest;

ANALYSE_FILE_END
