#include "AnlAnalyzerProcessor.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginBufferingAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::Processor::Processor(Vamp::Plugin* plugin)
: Vamp::HostExt::PluginWrapper(plugin)
{
}

size_t Analyzer::Processor::getWindowSize() const
{
    return mWindowSize;
}

size_t Analyzer::Processor::getStepSize() const
{
    return mStepSize;
}

Analyzer::Processor::ParameterList Analyzer::Processor::getParameterDescriptors() const
{
    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
    using PluginBufferingAdapter = Vamp::HostExt::PluginBufferingAdapter;
    auto* wrapper = dynamic_cast<PluginWrapper*>(m_plugin);
    
    auto parameters = m_plugin->getParameterDescriptors();
    if(wrapper == nullptr)
    {
        return parameters;
    }
    if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
    {
        if(wrapper->getWrapper<PluginBufferingAdapter>() == nullptr)
        {
            Vamp::Plugin::ParameterDescriptor descriptor;
            JUCE_COMPILER_WARNING("ensure the identifier is unique")
            descriptor.identifier = "z3";
            descriptor.name = "Window Increment";
            descriptor.description = "The window increment used to perform the FFT";
            descriptor.unit = "samples";
            for(unsigned int i = 8; i <= 65536; i *= 2)
            {
                descriptor.valueNames.push_back(std::to_string(i));
            }
            descriptor.minValue = 0.0f;
            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
            descriptor.defaultValue = 6.0f;
            descriptor.isQuantized = true;
            descriptor.quantizeStep = 1.0f;
            parameters.insert(parameters.begin(), descriptor);
        }

        {
            Vamp::Plugin::ParameterDescriptor descriptor;
            JUCE_COMPILER_WARNING("ensure the identifier is unique")
            descriptor.identifier = "z2";
            descriptor.name = "Window Size";
            descriptor.description = "The window size used to perform the FFT";
            descriptor.unit = "samples";
            for(unsigned int i = 8; i <= 65536; i *= 2)
            {
                descriptor.valueNames.push_back(std::to_string(i));
            }
            descriptor.minValue = 0.0f;
            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
            descriptor.defaultValue = 8.0f;
            descriptor.isQuantized = true;
            descriptor.quantizeStep = 1.0f;
            parameters.insert(parameters.begin(), descriptor);
        }
        {
            Vamp::Plugin::ParameterDescriptor descriptor;
            JUCE_COMPILER_WARNING("ensure the identifier is unique")
            descriptor.identifier = "z1";
            descriptor.name = "Window Type";
            descriptor.description = "The window type used to perform the FFT";
            descriptor.unit = "";
            descriptor.valueNames = {"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Blackman-Harris"};
            descriptor.minValue = 0.0f;
            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
            descriptor.defaultValue = 2.0f;
            descriptor.isQuantized = true;
            descriptor.quantizeStep = 1.0f;
            parameters.insert(parameters.begin(), descriptor);
        }
    }
    
    return parameters;
}

float Analyzer::Processor::getParameter(std::string identifier) const
{
    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
    using PluginBufferingAdapter = Vamp::HostExt::PluginBufferingAdapter;
    auto* wrapper = dynamic_cast<PluginWrapper*>(m_plugin);
    if(wrapper == nullptr)
    {
        return m_plugin->getParameter(identifier);
    }
    
    if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
    {
        if(identifier == "z1")
        {
            return static_cast<float>(adapter->getWindowType());
        }
        if(identifier == "z2")
        {
            return std::log(static_cast<float>(mWindowSize)) / std::log(2.0f) - 3.f;
        }
        if(identifier == "z3")
        {
            anlWeakAssert(wrapper->getWrapper<PluginBufferingAdapter>() == nullptr);
            return std::log(static_cast<float>(mStepSize)) / std::log(2.0f) - 3.f;
        }
    }
    
    return m_plugin->getParameter(identifier);
}

void Analyzer::Processor::setParameter(std::string identifier, float value)
{
    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
    using PluginBufferingAdapter = Vamp::HostExt::PluginBufferingAdapter;
    auto* wrapper = dynamic_cast<PluginWrapper*>(m_plugin);
    if(wrapper != nullptr)
    {
        if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
        {
            if(identifier == "z1")
            {
                adapter->setWindowType(static_cast<Vamp::HostExt::PluginInputDomainAdapter::WindowType>(std::floor(value)));
                return;
            }
            if(identifier == "z2")
            {
                anlWeakAssert(value >= 0.0f && value <= 13.0f);
                value = std::min(std::max(0.0f, value), 13.0f);
                mWindowSize = static_cast<size_t>(std::floor(std::pow(2.0f, (value + 3.f))));
                if(wrapper->getWrapper<PluginBufferingAdapter>() != nullptr)
                {
                    mStepSize = mWindowSize;
                }
                return;
            }
            if(identifier == "z3")
            {
                anlWeakAssert(value >= 0.0f && value <= 13.0f);
                value = std::min(std::max(0.0f, value), 13.0f);
                anlWeakAssert(adapter->getWrapper<PluginBufferingAdapter>() == nullptr);
                if(adapter->getWrapper<PluginBufferingAdapter>() != nullptr)
                {
                    return;
                }
                mStepSize = static_cast<size_t>(std::floor(std::pow(2.0f, (value + 3.f))));
                return;
            }
        }
    }
    
    
    m_plugin->setParameter(identifier, value);
}

std::unique_ptr<Analyzer::Processor> Analyzer::createProcessor(Accessor const& accessor, double sampleRate, AlertType alertType)
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
    
    auto pluginInstance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), static_cast<int>(sampleRate), PluginLoader::ADAPT_ALL_SAFE));
    if(pluginInstance == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    auto processor = std::make_unique<Processor>(pluginInstance.release());
    if(processor != nullptr)
    {
        for(auto const& parameter : accessor.getAttr<AttrType::parameters>())
        {
            processor->setParameter(parameter.first.toStdString(), static_cast<float>(parameter.second));
        }
    }
    return processor;
}

std::vector<Analyzer::Result> Analyzer::performAnalysis(Accessor const& accessor, juce::AudioFormatReader& audioFormatReader)
{
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const lengthInSamples = audioFormatReader.lengthInSamples;
    auto const sampleRate = audioFormatReader.sampleRate;
    auto const featureIndex = accessor.getAttr<AttrType::feature>();
    auto instance = createProcessor(accessor, sampleRate, AlertType::window);
    if(instance == nullptr)
    {
        return {};
    }
    
    auto const windowSize = instance->getWindowSize();
    auto const stepSize = instance->getStepSize();

    std::vector<Analyzer::Result> results;
    
    juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(windowSize));
    if(!instance->initialise(static_cast<size_t>(numChannels), stepSize, windowSize))
    {
        anlWeakAssert(false && "wrong initialization");
        return results;
    }
    for(juce::int64 timeStamp = 0; timeStamp < lengthInSamples; timeStamp += stepSize)
    {
        auto const remaininSamples = std::min(lengthInSamples - timeStamp, static_cast<juce::int64>(windowSize));
        audioFormatReader.read(buffer.getArrayOfWritePointers(), numChannels, timeStamp, static_cast<int>(remaininSamples));
        auto const rt = Vamp::RealTime::frame2RealTime(timeStamp, static_cast<unsigned int>(sampleRate));
        auto result = instance->process(buffer.getArrayOfReadPointers(), rt);
        auto it = result.find(static_cast<int>(featureIndex));
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
            results.insert(results.end(), it->second.begin(), it->second.end());
        }
    }
    auto result = instance->getRemainingFeatures();
    auto it = result.find(static_cast<int>(featureIndex));
    if(it != result.end())
    {
        results.insert(results.end(), it->second.cbegin(), it->second.cend());
    }
    
    return results;
}

ANALYSE_FILE_END
