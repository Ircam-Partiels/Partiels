#include "AnlPluginProcessor.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

Plugin::Processor::Processor(juce::AudioFormatReader& audioFormatReader, std::unique_ptr<Vamp::Plugin> plugin, size_t const feature, State const& state)
: mAudioFormatReader(audioFormatReader)
, mPlugin(std::move(plugin))
, mFeature(feature)
, mState(state)
, mBuffer(static_cast<int>(audioFormatReader.numChannels), static_cast<int>(state.blockSize))
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
    auto const numChannels = static_cast<int>(mAudioFormatReader.numChannels);
    auto const lengthInSamples = mAudioFormatReader.lengthInSamples;
    auto const sampleRate = mAudioFormatReader.sampleRate;
    auto const blockSize = mState.blockSize;
    auto const stepSize = mState.stepSize;
    anlStrongAssert(blockSize > 0 && stepSize > 0);
    if(blockSize <= 0 || stepSize <= 0)
    {
        return false;
    }
    
    auto** writePointers = mBuffer.getArrayOfWritePointers();
    auto const* const* readPointers = mBuffer.getArrayOfReadPointers();
    auto const position = mPosition;
    mPosition = position + static_cast<juce::int64>(stepSize);
    auto const rt = Vamp::RealTime::frame2RealTime(static_cast<long>(position), static_cast<unsigned int>(sampleRate));
    
    if(position > lengthInSamples)
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
            }
            results.insert(results.end(), it->second.cbegin(), it->second.cend());
        }
        return false;
    }
    
    auto const remaininSamples = std::min(lengthInSamples - position, static_cast<juce::int64>(blockSize));
    mAudioFormatReader.read(writePointers, numChannels, position, static_cast<int>(remaininSamples));

    auto result = mPlugin->process(readPointers, rt);
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
        }
        results.insert(results.end(), it->second.cbegin(), it->second.cend());
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

ANALYSE_FILE_END
