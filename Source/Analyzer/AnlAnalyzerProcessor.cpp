#include "AnlAnalyzerProcessor.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::unique_ptr<Vamp::Plugin> Analyzer::createPlugin(Accessor const& accessor, double sampleRate, bool showMessageOnFailure)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto const pluginKey = accessor.getValue<AttrType::key>();
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
        if(showMessageOnFailure)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin manager is not available.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    auto pluginInstance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), static_cast<int>(sampleRate), PluginLoader::ADAPT_ALL));
    if(pluginInstance == nullptr)
    {
        if(showMessageOnFailure)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    return pluginInstance;
}

void Analyzer::performAnalysis(Accessor& accessor, juce::AudioFormatReader& audioFormatReader, size_t blockSize)
{
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const lengthInSamples = audioFormatReader.lengthInSamples;
    auto const sampleRate = audioFormatReader.sampleRate;
    size_t featureIndex = 0;
    auto instance = createPlugin(accessor, sampleRate, true);
    if(instance == nullptr)
    {
        return;
    }
    
    auto const outputDescriptor = instance->getOutputDescriptors();
    
    auto getFeatureName = [&](size_t index)
    {
        if(outputDescriptor.size() > index)
        {
            return outputDescriptor[index].name;
        }
        return std::to_string(index);
    };
    
    auto printResult = [&](Vamp::Plugin::FeatureSet const& results)
    {
        for(auto const& channel : results)
        {
            std::cout << "Feature " << getFeatureName(static_cast<size_t>(channel.first)) << " - ";
            for(auto const& feature : channel.second)
            {
                std::cout << (feature.hasTimestamp ? "(" + feature.timestamp.toText(true) + ")" : "") << ": ";
                for(auto const& value : feature.values)
                {
                    std::cout << value << feature.label << " ";
                }
            }
            std::cout << "\n";
        }
    };
    
    using result_type = std::remove_const<std::remove_reference<decltype(accessor.getValue<AttrType::results>())>::type>::type;
    result_type results;
    
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
        printResult(result);
    }
    auto const result = instance->getRemainingFeatures();
    printResult(result);
    auto it = result.find(static_cast<int>(featureIndex));
    if(it != result.end())
    {
        results.insert(results.end(), it->second.begin(), it->second.end());
    }
    
    accessor.setValue<AttrType::results>(results, NotificationType::synchronous);
}

ANALYSE_FILE_END
