#include "AnlAnalyzerProcessor.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::unique_ptr<Vamp::Plugin> Analyzer::createPlugin(Accessor const& accessor, double sampleRate, AlertType alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto const pluginKey = accessor.getAttr<AttrType::key>();
    if(pluginKey.isEmpty())
    {
        return nullptr;
    }
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    
    using AlertIconType = juce::AlertWindow::AlertIconType;
    auto const errorMessage = juce::translate("Plugin cannot be loaded!");
    
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin manager is not available.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    auto pluginInstance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), static_cast<int>(sampleRate), PluginLoader::ADAPT_ALL));
    if(pluginInstance == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    return pluginInstance;
}

std::vector<Analyzer::Result> Analyzer::performAnalysis(Accessor const& accessor, juce::AudioFormatReader& audioFormatReader)
{
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const lengthInSamples = audioFormatReader.lengthInSamples;
    auto const sampleRate = audioFormatReader.sampleRate;
    auto const featureIndex = accessor.getAttr<AttrType::feature>();
    auto const blockSize = accessor.getAttr<AttrType::blockSize>();
    auto instance = createPlugin(accessor, sampleRate, AlertType::window);
    if(instance == nullptr)
    {
        return {};
    }

    std::vector<Analyzer::Result> results;
    
    juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(blockSize));
    instance->initialise(static_cast<size_t>(numChannels), blockSize, blockSize);
    for(juce::int64 timeStamp = 0; timeStamp < lengthInSamples; timeStamp += blockSize)
    {
        auto const remaininSamples = std::min(lengthInSamples - timeStamp, static_cast<juce::int64>(blockSize));
        audioFormatReader.read(buffer.getArrayOfWritePointers(), numChannels, timeStamp, static_cast<int>(remaininSamples));
        auto const result = instance->process(buffer.getArrayOfReadPointers(), Vamp::RealTime::frame2RealTime(timeStamp, static_cast<unsigned int>(sampleRate)));
        auto it = result.find(static_cast<int>(featureIndex));
        if(it != result.end())
        {
            results.insert(results.end(), it->second.begin(), it->second.end());
        }
    }
    auto const result = instance->getRemainingFeatures();
    auto it = result.find(static_cast<int>(featureIndex));
    if(it != result.end())
    {
        results.insert(results.end(), it->second.cbegin(), it->second.cend());
    }
    
    return results;
}

ANALYSE_FILE_END
