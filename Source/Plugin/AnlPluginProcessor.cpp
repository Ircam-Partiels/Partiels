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
    
    auto** writePointers = mBuffer.getArrayOfWritePointers();
    auto const* const* readPointers = mBuffer.getArrayOfReadPointers();
    auto const position = mPosition;
    mPosition = position + static_cast<juce::int64>(stepSize);
    
    if(position > lengthInSamples)
    {
        auto result = mPlugin->getRemainingFeatures();
        auto it = result.find(static_cast<int>(feature));
        if(it != result.end())
        {
            results.insert(results.end(), it->second.cbegin(), it->second.cend());
        }
        return false;
    }
    auto const remaininSamples = std::min(lengthInSamples - position, static_cast<juce::int64>(blockSize));
    
    mAudioFormatReader.read(writePointers, numChannels, position, static_cast<int>(remaininSamples));
    auto const rt = Vamp::RealTime::frame2RealTime(static_cast<long>(position), static_cast<unsigned int>(sampleRate));
    
    auto result = mPlugin->process(readPointers, rt);
    auto it = result.find(static_cast<int>(feature));
    if(it != result.end())
    {
        for(auto& that : it->second)
        {
            if(!that.hasTimestamp)
            {
                that.hasDuration = true;
                that.timestamp = rt;
            }
        }
        results.insert(results.end(), it->second.cbegin(), it->second.cend());
    }
    return true;
}

std::unique_ptr<Plugin::Processor> Plugin::Processor::create(Key const& key, State const& state, juce::AudioFormatReader& audioFormatReader, AlertType alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    using AlertIconType = juce::AlertWindow::AlertIconType;
    auto const errorTitle = juce::translate("Plugin cannot be loaded!");
    auto const errorMessage =  juce::translate("The plugin \"PLGNKEY: FTRKEY\" cannot be loaded: ").replace("PLGNKEY", key.identifier).replace("FTRKEY", key.feature);

    if(key.identifier.empty())
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorTitle, errorMessage + juce::translate("Key is not defined") + ".");
        }
        return nullptr;
    }
    
    if(key.feature.empty())
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorTitle, errorMessage + juce::translate(" Feature is not defined") + ".");
        }
        return nullptr;
    }
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorTitle, errorMessage + juce::translate(" Plugin manager is not available") + ".");
        }
        return nullptr;
    }
    
    std::unique_ptr<Vamp::Plugin> instance;
    try
    {
        instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(audioFormatReader.sampleRate), PluginLoader::ADAPT_ALL_SAFE));
    }
    catch(std::runtime_error& e)
    {
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorTitle, errorMessage + juce::translate(e.what()) + ".");
        return nullptr;
    }
    
    if(instance == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorTitle, errorMessage + juce::translate("Unknown reason") + ".");
        }
        return nullptr;
    }
    
    auto const outputs = instance->getOutputDescriptors();
    auto const feature = std::find_if(outputs.cbegin(), outputs.cend(), [&](auto const& output)
    {
        return output.identifier == key.feature;
    });
    if(feature == outputs.cend())
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, errorTitle, errorMessage + juce::translate("Invalid feature") + ".");
        }
        return nullptr;
    }
    auto const featureIndex = static_cast<size_t>(std::distance(outputs.cbegin(), feature));
    
    auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(instance.get());
    if(wrapper != nullptr)
    {
        if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
        {
            adapter->setWindowType(state.windowType);
        }
    }
    
    for(auto const& parameter : state.parameters)
    {
        instance->setParameter(parameter.first, parameter.second);
    }
    
    if(!instance->initialise(static_cast<size_t>(audioFormatReader.numChannels), state.stepSize, state.blockSize))
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, errorTitle, errorMessage + juce::translate("Initialization failed, either the number of channels, the step size or the block size might not be supported") + ".");
        }
        return nullptr;
    }

    return std::unique_ptr<Processor>(new Processor(audioFormatReader, std::move(instance), featureIndex, state));
}

ANALYSE_FILE_END
