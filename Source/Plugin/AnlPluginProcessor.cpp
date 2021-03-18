#include "AnlPluginProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

Plugin::Processor::CircularReader::CircularReader(juce::AudioFormatReader& audioFormatReader, size_t blockSize, size_t stepSize)
: mBlocksize(static_cast<int>(blockSize))
, mStepSize(static_cast<int>(stepSize))
, mAudioFormatReader(audioFormatReader)
, mBuffer(static_cast<int>(audioFormatReader.numChannels), mBlocksize * 2)
{
    mOutputBuffer.resize(static_cast<size_t>(audioFormatReader.numChannels));
}

double Plugin::Processor::CircularReader::getSampleRate() const
{
    return mAudioFormatReader.sampleRate;
}

bool Plugin::Processor::CircularReader::hasReachedEnd() const
{
    anlStrongAssert(mPosition >= static_cast<juce::int64>(0));
    anlStrongAssert(mPosition <= static_cast<juce::int64>(mAudioFormatReader.lengthInSamples));
    return mPosition >= static_cast<juce::int64>(mAudioFormatReader.lengthInSamples);
}

juce::int64 Plugin::Processor::CircularReader::getPosition() const
{
    return mPosition;
}

float const** Plugin::Processor::CircularReader::getNextBlock()
{
    auto** inputPointers = mBuffer.getArrayOfWritePointers();
    auto const numChannels = mBuffer.getNumChannels();
    
    mPosition = std::min(mPosition + static_cast<juce::int64>(mStepSize), mAudioFormatReader.lengthInSamples);
    
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
    auto const bufferSize = std::max(static_cast<int>(static_cast<juce::int64>(mBlocksize) - mReaderPosition), mStepSize);
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
    auto const silence = static_cast<int>(std::max(mReaderPosition + bufferSize - mAudioFormatReader.lengthInSamples, static_cast<juce::int64>(0)));
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
        mOutputBuffer[channel] = inputPointers[channel]+outputBufferPosition;
    }
    return mOutputBuffer.data();
}

Plugin::Processor::Processor(juce::AudioFormatReader& audioFormatReader, std::unique_ptr<Vamp::Plugin> plugin, size_t const feature, State const& state)
: mPlugin(std::move(plugin))
, mCircularReader(audioFormatReader, state.blockSize, state.stepSize)
, mFeature(feature)
, mState(state)
{
}

bool Plugin::Processor::performNextAudioBlock(std::vector<Result>& results)
{
    anlStrongAssert(mPlugin != nullptr);
    if(mPlugin == nullptr)
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
        JUCE_COMPILER_WARNING("the time stamp is not always working")
        auto result = mPlugin->getRemainingFeatures();
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
                results.emplace_back(std::move(that));
            }
        }
        return false;
    }
    
    auto result = mPlugin->process(mCircularReader.getNextBlock(), rt);
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
            results.emplace_back(std::move(that));
        }
    }
    return true;
}

Plugin::Description Plugin::Processor::getDescription() const
{
    anlStrongAssert(mPlugin != nullptr);
    if(mPlugin == nullptr)
    {
        return {};
    }
    
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {};
    }

    Plugin::Description description;
    description.name = mPlugin->getName();
    description.inputDomain = Vamp::Plugin::InputDomain::TimeDomain;
    if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(mPlugin.get()))
    {
        using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
        if(auto* inputDomainAdapter = wrapper->getWrapper<PluginInputDomainAdapter const>())
        {
            description.inputDomain = Vamp::Plugin::InputDomain::FrequencyDomain;
            description.defaultState.windowType = inputDomainAdapter->getWindowType();
        }
    }

    description.maker = mPlugin->getMaker();
    description.version = static_cast<unsigned int>(mPlugin->getPluginVersion());
    auto const categories = pluginLoader->getPluginCategory(mPlugin->getIdentifier());
    description.category = categories.empty() ? "": categories.front();
    description.details = mPlugin->getDescription();

    auto const blockSize = mPlugin->getPreferredBlockSize();
    description.defaultState.blockSize = blockSize > 0 ? blockSize : 512;
    auto const stepSize = mPlugin->getPreferredStepSize();
    description.defaultState.stepSize = stepSize > 0 ? stepSize : description.defaultState.blockSize;
    auto const parameters = mPlugin->getParameterDescriptors();
    description.parameters.insert(description.parameters.cbegin(), parameters.cbegin(), parameters.cend());
    for(auto const& parameter : parameters)
    {
        description.defaultState.parameters[parameter.identifier] = parameter.defaultValue;
    }

    auto const descriptors = mPlugin->getOutputDescriptors();
    anlStrongAssert(descriptors.size() > mFeature);
    description.output = descriptors.size() > mFeature ? descriptors[mFeature] : Plugin::Output{};

    return description;
}

std::unique_ptr<Plugin::Processor> Plugin::Processor::create(Key const& key, State const& state, juce::AudioFormatReader& audioFormatReader)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }
    
    anlStrongAssert(!key.identifier.empty());
    if(key.identifier.empty())
    {
        throw std::invalid_argument("plugin key is not defined");
    }
    anlStrongAssert(!key.feature.empty());
    if(key.feature.empty())
    {
        throw std::invalid_argument("plugin feature is not defined");
    }
    anlStrongAssert(state.blockSize > 0);
    if(state.blockSize == 0)
    {
        throw std::invalid_argument("block size cannot be null");
    }
    anlStrongAssert(state.stepSize > 0);
    if(state.stepSize == 0)
    {
        throw std::invalid_argument("step size cannot be null");
    }
    
    auto instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(audioFormatReader.sampleRate), Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE));
    if(instance == nullptr)
    {
        throw std::runtime_error("allocation failed");
    }
    
    auto const outputs = instance->getOutputDescriptors();
    auto const feature = std::find_if(outputs.cbegin(), outputs.cend(), [&](auto const& output)
    {
        return output.identifier == key.feature;
    });
    if(feature == outputs.cend())
    {
        throw std::runtime_error("plugin feature is not invalid");
    }
    
    auto const featureIndex = static_cast<size_t>(std::distance(outputs.cbegin(), feature));
    auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(instance.get());
    if(wrapper != nullptr)
    {
        if(auto* adapter = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
        {
            adapter->setWindowType(state.windowType);
        }
    }
    
    auto const descriptors = instance->getParameterDescriptors();
    for(auto const& parameter : state.parameters)
    {
        if(std::any_of(descriptors.cbegin(), descriptors.cend(), [&](auto const& descriptor)
        {
            return descriptor.identifier == parameter.first;
        }))
        {
            instance->setParameter(parameter.first, parameter.second);
        }
        else
        {
            throw std::invalid_argument("invalid parameters");
        }
    }
    
    if(!instance->initialise(static_cast<size_t>(audioFormatReader.numChannels), state.stepSize, state.blockSize))
    {
        return nullptr;
    }

    auto processor = std::unique_ptr<Processor>(new Processor(audioFormatReader, std::move(instance), featureIndex, state));
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
    
    CircularReaderUnitTest() : juce::UnitTest("CircularReader", "Plugin") {}
    
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
            
            bool readSamples(int** destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64 startSampleInFile, int numSamples) override
            {
                for(int channelIndex = 0; channelIndex < numDestChannels; ++channelIndex)
                {
                    auto* channel = reinterpret_cast<float*>(destChannels[channelIndex])+startOffsetInDestBuffer;
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
            while(!circularReader.hasReachedEnd())
            {
                position = circularReader.getPosition();
                output = circularReader.getNextBlock();
                for(size_t sampleIndex = 0_z; sampleIndex < blockSize; ++sampleIndex)
                {
                    auto const readerPosition = position + static_cast<juce::int64>(sampleIndex);
                    auto const expected = readerPosition < reader.lengthInSamples ? static_cast<float>(readerPosition) : 0.0f;
                    expectWithinAbsoluteError(output[0_z][sampleIndex], expected, 0.001f,
                                              "Position " + juce::String(position) + " " + juce::String(readerPosition));
                }
            }
            expectEquals(circularReader.getPosition(), reader.lengthInSamples);
        };
        
        performTest(1024, 1024);
        performTest(1024, 2000);
        performTest(1024, 256);
        performTest(1024, 333);
    }
};

static Plugin::Processor::CircularReaderUnitTest pluginProcessorCircularReaderUnitTest;

ANALYSE_FILE_END
