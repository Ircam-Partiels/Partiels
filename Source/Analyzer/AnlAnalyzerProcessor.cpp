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
    auto const featureIndex = accessor.getValue<AttrType::feature>();
    auto instance = createPlugin(accessor, sampleRate, true);
    if(instance == nullptr)
    {
        return;
    }
    
    auto const outputDescriptor = instance->getOutputDescriptors();
    anlStrongAssert(featureIndex < outputDescriptor.size());
    if(featureIndex >= outputDescriptor.size())
    {
        return;
    }
    
    auto const numDimension = std::min(outputDescriptor[featureIndex].binCount, 2ul) + 1;
    
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
    }
    auto const result = instance->getRemainingFeatures();
    auto it = result.find(static_cast<int>(featureIndex));
    if(it != result.end())
    {
        results.insert(results.end(), it->second.cbegin(), it->second.cend());
    }
    if(numDimension == 2)
    {
        auto pair = std::minmax_element(results.cbegin(), results.cend(), [](auto const& lhs, auto const& rhs)
        {
            return lhs.values[0] < rhs.values[0];
        });
        auto const min = static_cast<double>(pair.first->values[0]);
        auto const max = static_cast<double>(pair.second->values[0]);
        auto& zoomAcsr = accessor.getAccessors<AttrType::zoom>()[0].get();
        zoomAcsr.setValue<Zoom::AttrType::globalRange>(juce::Range<double>{min, max}, NotificationType::synchronous);
        zoomAcsr.setValue<Zoom::AttrType::visibleRange>(juce::Range<double>{min, max}, NotificationType::synchronous);
    }
    
    accessor.setValue<AttrType::results>(results, NotificationType::synchronous);
}

ANALYSE_FILE_END
