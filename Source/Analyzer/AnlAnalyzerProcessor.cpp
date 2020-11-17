#include "AnlAnalyzerProcessor.h"
#include "AnlAnalyzerPluginManager.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::Processor::Processor(Accessor& accessor)
: mAccessor(accessor)
{
    
}

void Analyzer::Processor::perform(juce::AudioFormatReader& audioFormatReader, size_t blockSize)
{
    auto instance = PluginManager::createInstance(mAccessor, true);
    if(instance == nullptr)
    {
        return;
    }
    
    auto printResult = [](Vamp::Plugin::FeatureSet const& results)
    {
        for(auto const& channel : results)
        {
            std::cout << "Channel " << channel.first << " - ";
            for(auto const& feature : channel.second)
            {
                std::cout << feature.label << ": ";
                for(auto const& value : feature.values)
                {
                    std::cout << value << " ";
                }
            }
            std::cout << "\n";
        }
    };
    
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const lengthInSamples = audioFormatReader.lengthInSamples;
    
    juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(blockSize));
    instance->initialise(static_cast<size_t>(numChannels), blockSize, blockSize);
    for(juce::int64 timeStamp = 0; timeStamp < lengthInSamples; timeStamp += blockSize)
    {
        auto const remaininSamples = std::min(lengthInSamples - timeStamp, static_cast<juce::int64>(blockSize));
        audioFormatReader.read(buffer.getArrayOfWritePointers(), numChannels, timeStamp, static_cast<int>(remaininSamples));
        auto const result = instance->process(buffer.getArrayOfReadPointers(), Vamp::RealTime::frame2RealTime(timeStamp, instance->getInputSampleRate()));
        printResult(result);
    }
    printResult(instance->getRemainingFeatures());
}

ANALYSE_FILE_END
