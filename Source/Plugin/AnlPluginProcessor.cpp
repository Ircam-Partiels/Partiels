#include "AnlPluginProcessor.h"
#include "AnlPluginTools.h"

ANALYSE_FILE_BEGIN

Plugin::Processor::CircularReader::CircularReader(juce::AudioFormatReader& audioFormatReader, size_t blockSize, size_t stepSize)
: mBlocksize(static_cast<int>(blockSize))
, mStepSize(static_cast<int>(stepSize))
, mOffset(static_cast<juce::int64>(std::round(static_cast<double>(blockSize) / 2.0)))
, mAudioFormatReader(audioFormatReader)
, mBuffer(static_cast<int>(audioFormatReader.numChannels), mBlocksize * 2)
, mReaderPosition(-mOffset)
{
    mOutputBuffer.resize(static_cast<size_t>(audioFormatReader.numChannels));
}

size_t Plugin::Processor::CircularReader::getNumChannels() const
{
    return static_cast<size_t>(mAudioFormatReader.numChannels);
}

juce::int64 Plugin::Processor::CircularReader::getLengthInSamples() const
{
    return mAudioFormatReader.lengthInSamples;
}

double Plugin::Processor::CircularReader::getSampleRate() const
{
    return mAudioFormatReader.sampleRate;
}

juce::int64 Plugin::Processor::CircularReader::getOffsetInSamples() const
{
    return mOffset;
}

bool Plugin::Processor::CircularReader::hasReachedEnd() const
{
    MiscWeakAssert(mPosition >= static_cast<juce::int64>(0));
    MiscWeakAssert(mPosition <= mAudioFormatReader.lengthInSamples + mOffset);
    return mPosition >= mAudioFormatReader.lengthInSamples + mOffset;
}

juce::int64 Plugin::Processor::CircularReader::getPosition() const
{
    return mPosition;
}

float const** Plugin::Processor::CircularReader::getNextBlock()
{
    auto* const* inputPointers = mBuffer.getArrayOfWritePointers();
    auto const numChannels = mBuffer.getNumChannels();

    auto const maxLength = mAudioFormatReader.lengthInSamples + mOffset;
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
    auto const bufferSize = std::max(static_cast<int>(static_cast<juce::int64>(mBlocksize) - (mReaderPosition + mOffset)), mStepSize);
    // The number of sample that can be used from the previous block
    auto const hopeSize = std::max(mBlocksize - bufferSize, 0);

    auto bufferPosition = mBufferPosition;
    if(bufferPosition + mStepSize > mBuffer.getNumSamples())
    {
        auto const bufferSource = bufferPosition - hopeSize;
        MiscStrongAssert(bufferSource != bufferPosition);
        if(bufferSource != bufferPosition)
        {
            for(int channel = 0; channel < numChannels; ++channel)
            {
                mBuffer.copyFrom(channel, 0, mBuffer.getReadPointer(channel, bufferSource), hopeSize);
            }
        }
        mBuffer.clear(hopeSize, mBuffer.getNumSamples() - hopeSize);
        bufferPosition = hopeSize;
        MiscStrongAssert(bufferSize == static_cast<juce::int64>(mStepSize));
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

Plugin::Processor::Processor(juce::AudioFormatReader& audioFormatReader, std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins, Key const& key, std::vector<std::string> const& features, std::vector<size_t> const& featureIndices, State const& state, VoiceCounts const& voiceCounts)
: mPlugins(std::move(plugins))
, mCircularReader(audioFormatReader, state.blockSize, state.stepSize)
, mKey(key)
, mFeatures(features)
, mFeatureIndices(featureIndices)
, mState(state)
, mVoiceCounts(voiceCounts)
{
}

juce::Result Plugin::Processor::prepareToAnalyze(Results& results)
{
    MiscStrongAssert(!mPlugins.empty());
    if(mPlugins.empty())
    {
        return juce::Result::fail(juce::translate("The processor has no plugin to process the audio"));
    }
    if(std::any_of(mPlugins.cbegin(), mPlugins.cend(), [](auto const& plugin)
                   {
                       return plugin == nullptr;
                   }))
    {
        return juce::Result::fail(juce::translate("The processor has a null plugin"));
    }

    auto const blockSize = mState.blockSize;
    auto const stepSize = mState.stepSize;
    MiscStrongAssert(blockSize > 0 && stepSize > 0);
    if(blockSize <= 0 || stepSize <= 0)
    {
        return juce::Result::fail(juce::translate("The processor has an invalid block or step size"));
    }

    auto numReaderChannels = mCircularReader.getNumChannels();
    auto const maxChannels = mPlugins.at(0)->getMaxChannelCount();
    for(auto& plugin : mPlugins)
    {
        if(auto* adapter = plugin->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
        {
            adapter->setWindowType(mState.windowType);
        }

        // The numbers of voices of the first plugin are already set (and its outputs retrieved)
        if(plugin != mPlugins.front())
        {
            Tools::setVoiceCounts(*plugin.get(), mVoiceCounts);
        }
        Tools::setParameters(*plugin.get(), mState);

        auto const numChannels = std::min(maxChannels, numReaderChannels);
        numReaderChannels -= numChannels;
        if(!plugin->initialise(numChannels, mState.stepSize, mState.blockSize))
        {
            return juce::Result::fail(juce::translate("The processor failed to initialise the plugin"));
        }

        // The window depends on the block size so it is only available after the initialisation
        if(auto* adapter = plugin->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
        {
            plugin->setFrequencyDomainWindow(adapter->getWindowCoefficients());
        }
    }

    auto const descriptors = mPlugins.at(0)->getOutputDescriptors();
    MiscStrongAssert(!mFeatureIndices.empty());
    if(mFeatureIndices.empty())
    {
        return juce::Result::fail(juce::translate("The processor has no feature to analyze"));
    }
    if(std::any_of(mFeatureIndices.cbegin(), mFeatureIndices.cend(), [&](auto const featureIndex)
                   {
                       return featureIndex >= descriptors.size();
                   }))
    {
        MiscStrongAssert(false && "invalid feature index");
        return juce::Result::fail(juce::translate("The processor has an invalid feature index"));
    }

    results.clear();
    results.resize(mFeatureIndices.size());
    for(auto featureIndex = 0_z; featureIndex < mFeatureIndices.size(); ++featureIndex)
    {
        auto& featureResults = results[featureIndex];
        featureResults.resize(mPlugins.size());
        auto const& descriptor = descriptors[mFeatureIndices[featureIndex]];
        if(descriptor.sampleType == Output::SampleType::OneSamplePerStep)
        {
            auto const length = static_cast<double>(mCircularReader.getLengthInSamples());
            auto const size = static_cast<size_t>(std::ceil(length / static_cast<double>(mState.stepSize)));
            for(auto& channelResults : featureResults)
            {
                channelResults.reserve(size);
            }
        }
        else if(descriptor.sampleType == Output::SampleType::FixedSampleRate)
        {
            auto const length = static_cast<double>(mCircularReader.getLengthInSamples());
            auto const duration = length / mCircularReader.getSampleRate();
            auto const size = static_cast<size_t>(std::ceil(duration * descriptor.sampleRate));
            for(auto& channelResults : featureResults)
            {
                channelResults.reserve(size);
            }
        }
    }
    return juce::Result::ok();
}

juce::Result Plugin::Processor::setPrecomputingResults(std::vector<std::vector<std::vector<Result>>> const& results)
{
    MiscStrongAssert(!mPlugins.empty());
    if(mPlugins.empty())
    {
        return juce::Result::fail(juce::translate("The processor has no plugin to process the audio"));
    }
    if(std::any_of(mPlugins.cbegin(), mPlugins.cend(), [](auto const& plugin)
                   {
                       return plugin == nullptr;
                   }))
    {
        return juce::Result::fail(juce::translate("The processor has a null plugin"));
    }

    MiscWeakAssert(results.empty() || results.size() == 1_z || results.size() == mPlugins.size());
    if(results.size() == 1_z)
    {
        Vamp::Plugin::FeatureSet fs;
        for(auto featureIndex = 0_z; featureIndex < results.at(0_z).size(); ++featureIndex)
        {
            fs[static_cast<int>(featureIndex)] = results.at(0_z).at(featureIndex);
        }
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            mPlugins[index]->setPreComputingFeatures(fs);
        }
    }
    else if(results.size() == mPlugins.size())
    {
        for(size_t channelIndex = 0; channelIndex < mPlugins.size(); ++channelIndex)
        {
            Vamp::Plugin::FeatureSet fs;
            for(auto featureIndex = 0_z; featureIndex < results.at(channelIndex).size(); ++featureIndex)
            {
                fs[static_cast<int>(featureIndex)] = results.at(channelIndex).at(featureIndex);
            }
            mPlugins[channelIndex]->setPreComputingFeatures(fs);
        }
    }
    else if(!results.empty())
    {
        Vamp::Plugin::FeatureSet fs;
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            mPlugins[index]->setPreComputingFeatures(fs);
        }
    }
    return (results.empty() || results.size() == 1_z || results.size() == mPlugins.size()) ? juce::Result::ok() : juce::Result::fail(juce::translate("The precomputed results size is invalid"));
}

std::tuple<juce::Result, bool> Plugin::Processor::performNextAudioBlock(Results& results)
{
    MiscStrongAssert(!mPlugins.empty());
    if(mPlugins.empty())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The processor has no plugin to process the audio")), false);
    }
    if(std::any_of(mPlugins.cbegin(), mPlugins.cend(), [](auto const& plugin)
                   {
                       return plugin == nullptr;
                   }))
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The processor has a null plugin")), false);
    }
    MiscStrongAssert(results.size() == mFeatureIndices.size() && std::all_of(results.cbegin(), results.cend(), [this](auto const& featureResults)
                                                                             {
                                                                                 return featureResults.size() == mPlugins.size();
                                                                             }));
    if(results.size() != mFeatureIndices.size() || std::any_of(results.cbegin(), results.cend(), [this](auto const& featureResults)
                                                               {
                                                                   return featureResults.size() != mPlugins.size();
                                                               }))
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The processor results are not prepared")), false);
    }

    auto const blockSize = mState.blockSize;
    auto const stepSize = mState.stepSize;
    MiscStrongAssert(blockSize > 0 && stepSize > 0);
    if(blockSize <= 0 || stepSize <= 0)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("The processor has an invalid block or step size")), false);
    }

    auto const position = mCircularReader.getPosition();
    auto const sampleRate = mCircularReader.getSampleRate();
    auto const rt = Vamp::RealTime::frame2RealTime(static_cast<long>(position), static_cast<unsigned int>(sampleRate));

    auto const addResults = [&](Vamp::Plugin::FeatureSet& featureSet, size_t channelIndex)
    {
        for(auto featureIndex = 0_z; featureIndex < mFeatureIndices.size(); ++featureIndex)
        {
            auto it = featureSet.find(static_cast<int>(mFeatureIndices[featureIndex]));
            if(it != featureSet.end())
            {
                auto& channelResults = results[featureIndex][channelIndex];
                for(auto& that : it->second)
                {
                    if(!that.hasTimestamp)
                    {
                        that.hasTimestamp = true;
                        that.timestamp = rt;
                    }
                    channelResults.emplace_back(std::move(that));
                }
            }
        }
    };

    if(mCircularReader.hasReachedEnd())
    {
        for(size_t index = 0; index < mPlugins.size(); ++index)
        {
            auto featureSet = mPlugins[index]->getRemainingFeatures();
            addResults(featureSet, index);
        }
        return std::make_tuple(juce::Result::ok(), false);
    }

    auto const** block = mCircularReader.getNextBlock();
    for(size_t index = 0; index < mPlugins.size(); ++index)
    {
        auto featureSet = mPlugins[index]->process(block, rt);
        addResults(featureSet, index);
        block += mPlugins[index]->getMaxChannelCount();
    }
    return std::make_tuple(juce::Result::ok(), true);
}

float Plugin::Processor::getAdvancement() const
{
    auto const position = static_cast<float>(mCircularReader.getPosition());
    auto const length = static_cast<float>(mCircularReader.getLengthInSamples()) + std::round(static_cast<float>(mState.blockSize) / 2.0f);
    return position / length;
}

size_t Plugin::Processor::getNumFeatures() const
{
    return mFeatures.size();
}

Plugin::Description Plugin::Processor::getDescription(size_t featureIndex) const
{
    MiscStrongAssert(!mPlugins.empty());
    MiscStrongAssert(featureIndex < mFeatures.size());
    if(mPlugins.empty() || mPlugins.at(0) == nullptr || featureIndex >= mFeatures.size())
    {
        return {};
    }
    return loadDescription(*mPlugins.at(0), Key{mKey.identifier, mFeatures.at(featureIndex)});
}

std::vector<Plugin::Input> Plugin::Processor::getInputs() const
{
    MiscStrongAssert(!mPlugins.empty());
    if(mPlugins.empty() || mPlugins.at(0) == nullptr)
    {
        return {};
    }

    auto const descriptors = mPlugins.at(0)->getInputDescriptors();
    std::vector<Plugin::Input> inputs;
    inputs.reserve(descriptors.size());
    for(auto const& descriptor : descriptors)
    {
        inputs.push_back(descriptor);
    }
    return inputs;
}

Plugin::Output Plugin::Processor::getOutput(size_t featureIndex) const
{
    MiscStrongAssert(!mPlugins.empty());
    MiscStrongAssert(featureIndex < mFeatureIndices.size());
    if(mPlugins.empty() || mPlugins.at(0) == nullptr || featureIndex >= mFeatureIndices.size())
    {
        return {};
    }

    auto const descriptors = mPlugins.at(0)->getOutputDescriptors();
    auto const descriptorIndex = mFeatureIndices.at(featureIndex);
    MiscStrongAssert(descriptorIndex < descriptors.size());
    return descriptorIndex < descriptors.size() ? descriptors.at(descriptorIndex) : Plugin::Output{};
}

std::unique_ptr<Plugin::Processor> Plugin::Processor::create(Key const& key, State const& state, VoiceCounts const& voiceCounts, juce::AudioFormatReader& audioFormatReader, std::vector<std::string> const& features)
{
    auto plugins = Tools::createPluginWrappers(key, state, static_cast<size_t>(audioFormatReader.numChannels), audioFormatReader.sampleRate);
    if(plugins.empty())
    {
        return nullptr;
    }
    // The outputs can depend on the parameters and the numbers of voices so they must be set before retrieving the feature indices
    auto& plugin = *plugins.at(0_z).get();
    Tools::setVoiceCounts(plugin, voiceCounts);
    Tools::setParameters(plugin, state);
    auto const effectiveFeatures = features.empty() ? std::vector<std::string>{key.feature} : features;
    std::vector<size_t> featureIndices;
    featureIndices.reserve(effectiveFeatures.size());
    for(auto const& feature : effectiveFeatures)
    {
        auto const featureIndex = Tools::getFeatureIndex(plugin, feature);
        if(!featureIndex.has_value())
        {
            throw LoadingError("plugin feature is invalid");
        }
        featureIndices.push_back(featureIndex.value());
    }
    return std::unique_ptr<Processor>(new Processor(audioFormatReader, std::move(plugins), key, effectiveFeatures, featureIndices, state, voiceCounts));
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
            auto const offset = circularReader.getOffsetInSamples();
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

class Plugin::Processor::FeaturesUnitTest
: public juce::UnitTest
{
public:
    FeaturesUnitTest()
    : juce::UnitTest("Features", "Plugin")
    {
    }

    ~FeaturesUnitTest() override = default;

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

            bool readSamples(int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer, juce::int64, int numSamples) override
            {
                for(int channelIndex = 0; channelIndex < numDestChannels; ++channelIndex)
                {
                    auto* channel = reinterpret_cast<float*>(destChannels[channelIndex]) + startOffsetInDestBuffer;
                    std::fill(channel, channel + numSamples, 0.0f);
                }
                return true;
            }
        };

        // The test requires the IVE example plugins (VAMP_PATH must contain their directory)
        static auto const identifier = std::string("ive_plugin_example:ivemultivoices");
        auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
        auto const pluginKeys = pluginLoader != nullptr ? pluginLoader->listPlugins(false) : std::vector<std::string>{};
        if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), identifier) == pluginKeys.cend())
        {
            logMessage("The plugin " + identifier + " cannot be found, the test is skipped");
            return;
        }

        Reader reader(44100.0, 44100 * 2);
        State state;
        state.blockSize = 256_z;
        state.stepSize = 256_z;
        VoiceCounts const voiceCounts{{"voice1", 5_z}};

        auto const analyze = [&](std::unique_ptr<Processor> processor)
        {
            Results results;
            expect(processor != nullptr);
            if(processor == nullptr)
            {
                return results;
            }
            expect(processor->prepareToAnalyze(results).wasOk());
            auto performResult = processor->performNextAudioBlock(results);
            while(std::get<1>(performResult))
            {
                performResult = processor->performNextAudioBlock(results);
            }
            expect(std::get<0>(performResult).wasOk());
            return results;
        };

        beginTest("features depending on the numbers of voices");
        {
            expectDoesNotThrow(Processor::create({identifier, "voice5"}, state, voiceCounts, reader));
            expectThrowsType(Processor::create({identifier, "voice6"}, state, voiceCounts, reader), LoadingError);
            expectThrowsType(Processor::create({identifier, "voice4"}, state, {}, reader), LoadingError);
            expectThrowsType(Processor::create({identifier, "voice2"}, state, {{"voice1", 1_z}}, reader), LoadingError);
            // The output that follows the voices is moved
            auto processor = Processor::create({identifier, "active"}, state, voiceCounts, reader);
            expect(processor != nullptr && processor->getDescription().output.identifier == "active");
            expect(processor != nullptr && !processor->getDescription().hasVoices());
            expect(processor != nullptr && Tools::getVoicesName(processor->getDescription()) == "Active Voices");
        }

        beginTest("description of the voices");
        {
            auto processor = Processor::create({identifier, "voice1"}, state, {{"voice1", 1_z}}, reader);
            expect(processor != nullptr);
            if(processor != nullptr)
            {
                // An output with a single voice keeps its voices (so the number of voices can be changed)
                auto const description = processor->getDescription();
                expect(description.hasVoices());
                expect(description.voices == std::vector<std::string>{"voice1"});
                expectEquals(description.voiceDescriptor.identifier, std::string("voices"));
                expectEquals(description.voiceDescriptor.minValue, 1.0f);
                expectEquals(description.voiceDescriptor.maxValue, 8.0f);
                expectEquals(description.voiceDescriptor.defaultValue, 3.0f);
                expectEquals(Tools::getVoicesName(description), juce::String("Voice"));
            }
        }

        beginTest("several features analyzed by the same processor");
        {
            std::vector<std::string> const features{"voice1", "voice2", "voice3", "voice4", "voice5"};
            auto processor = Processor::create({identifier, "voice1"}, state, voiceCounts, reader, features);
            expect(processor != nullptr);
            if(processor == nullptr)
            {
                return;
            }
            expectEquals(processor->getNumFeatures(), features.size());
            for(auto featureIndex = 0_z; featureIndex < features.size(); ++featureIndex)
            {
                auto const description = processor->getDescription(featureIndex);
                expect(description.output.identifier == features.at(featureIndex));
                expect(description.voices == features);
            }

            auto const results = analyze(std::move(processor));
            expectEquals(results.size(), features.size());
            for(auto featureIndex = 0_z; featureIndex < std::min(results.size(), features.size()); ++featureIndex)
            {
                // The results of a feature must be the same as with a processor that analyzes only this feature
                auto const expected = analyze(Processor::create({identifier, features.at(featureIndex)}, state, voiceCounts, reader));
                expectEquals(expected.size(), 1_z);
                expectEquals(results.at(featureIndex).size(), 1_z);
                if(!expected.empty() && expected.front().size() == 1_z && results.at(featureIndex).size() == 1_z)
                {
                    auto const& featureResults = results.at(featureIndex).front();
                    expect(!featureResults.empty());
                    auto const& expectedResults = expected.front().front();
                    auto const equals = featureResults.size() == expectedResults.size() && std::equal(featureResults.cbegin(), featureResults.cend(), expectedResults.cbegin(), [](Result const& lhs, Result const& rhs)
                                                                                                      {
                                                                                                          return lhs == rhs;
                                                                                                      });
                    expect(equals, "voice " + juce::String(featureIndex + 1_z));
                }
            }
        }
    }
};

static Plugin::Processor::FeaturesUnitTest pluginProcessorFeaturesUnitTest;

ANALYSE_FILE_END
