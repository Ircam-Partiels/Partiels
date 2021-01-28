#include "AnlAnalyzerProcessor.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::Processor::Processor(std::unique_ptr<Vamp::Plugin> plugin, size_t feature)
: mPlugin(std::move(plugin))
, mFeature(feature)
{
    if(mPlugin == nullptr)
    {
        return;
    }
    
    auto const& descriptors = mPlugin->getParameterDescriptors();
    auto getNextIdentifier = [&](std::string const& prepend)
    {
        int index = 0;
        while(std::any_of(descriptors.cbegin(), descriptors.cend(), [&](auto const& descriptor)
        {
            return descriptor.identifier == prepend + std::to_string(index);
        }))
        {
            ++index;
        }
        return prepend + std::to_string(index);
    };
    mIdentifierWindowType = getNextIdentifier("windowType");
    mIdentifierWindowSize = getNextIdentifier("windowSize");
    mIdentifierWindowOverlapping = getNextIdentifier("windowOverlapping");
}

double Analyzer::Processor::getSampleRate() const
{
    anlStrongAssert(mPlugin != nullptr);
    if(mPlugin == nullptr)
    {
        return 0.0;
    }
    return static_cast<double>(mPlugin->getInputSampleRate());
}

Analyzer::Processor::Description Analyzer::Processor::getDescription() const
{
    Description description;
    anlStrongAssert(mPlugin != nullptr);
    if(mPlugin == nullptr)
    {
        return description;
    }
    
    description.name = mPlugin->getName();
    description.maker = mPlugin->getMaker();
    description.version = static_cast<unsigned int>(mPlugin->getPluginVersion());
    
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader != nullptr)
    {
        auto const categories = pluginLoader->getPluginCategory(mPlugin->getIdentifier());
        description.category = categories.empty() ? "": categories.front();
    }

    description.details = mPlugin->getDescription();
    auto const outputs = mPlugin->getOutputDescriptors();
    anlStrongAssert(outputs.size() > mFeature);
    if(outputs.size() > mFeature)
    {
        description.output = outputs[mFeature];
    }
    return description;
}

//Vamp::Plugin::ParameterList Analyzer::Processor::getParameterDescriptors() const
//{
//    if(mPlugin == nullptr)
//    {
//        return;
//    }
//    
//    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
//    using PluginBufferingAdapter = Vamp::HostExt::PluginBufferingAdapter;
//    auto* wrapper = dynamic_cast<PluginWrapper*>(m_plugin);
//    
//    auto parameters = m_plugin->getParameterDescriptors();
//    if(wrapper == nullptr)
//    {
//        return parameters;
//    }
//    if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
//    {
//        if(wrapper->getWrapper<PluginBufferingAdapter>() == nullptr)
//        {
//            Vamp::Plugin::ParameterDescriptor descriptor;
//            descriptor.identifier = mIdentifierWindowOverlapping;
//            descriptor.name = "Window Overlapping";
//            descriptor.description = "The window overlapping used to perform the FFT";
//            descriptor.unit = "x";
//            for(unsigned int i = 1; i <= 16; i *= 2)
//            {
//                descriptor.valueNames.push_back(std::to_string(i));
//            }
//            descriptor.minValue = 0.0f;
//            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
//            descriptor.defaultValue = 4.0f;
//            descriptor.isQuantized = true;
//            descriptor.quantizeStep = 1.0f;
//            parameters.insert(parameters.begin(), descriptor);
//        }
//
//        {
//            Vamp::Plugin::ParameterDescriptor descriptor;
//            descriptor.identifier = mIdentifierWindowSize;
//            descriptor.name = "Window Size";
//            descriptor.description = "The window size used to perform the FFT";
//            descriptor.unit = "samples";
//            for(unsigned int i = 8; i <= 65536; i *= 2)
//            {
//                descriptor.valueNames.push_back(std::to_string(i));
//            }
//            descriptor.minValue = 0.0f;
//            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
//            descriptor.defaultValue = 8.0f;
//            descriptor.isQuantized = true;
//            descriptor.quantizeStep = 1.0f;
//            parameters.insert(parameters.begin(), descriptor);
//        }
//        {
//            Vamp::Plugin::ParameterDescriptor descriptor;
//            descriptor.identifier = mIdentifierWindowType;
//            descriptor.name = "Window Type";
//            descriptor.description = "The window type used to perform the FFT";
//            descriptor.unit = "";
//            descriptor.valueNames = {"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Blackman-Harris"};
//            descriptor.minValue = 0.0f;
//            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
//            descriptor.defaultValue = 2.0f;
//            descriptor.isQuantized = true;
//            descriptor.quantizeStep = 1.0f;
//            parameters.insert(parameters.begin(), descriptor);
//        }
//    }
//    
//    return parameters;
//}

//float Analyzer::Processor::getParameter(std::string identifier) const
//{
//    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
//    using PluginBufferingAdapter = Vamp::HostExt::PluginBufferingAdapter;
//    auto* wrapper = dynamic_cast<PluginWrapper*>(m_plugin);
//    if(wrapper == nullptr)
//    {
//        return m_plugin->getParameter(identifier);
//    }
//
//    if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
//    {
//        if(identifier == mIdentifierWindowType)
//        {
//            return static_cast<float>(adapter->getWindowType());
//        }
//        if(identifier == mIdentifierWindowSize)
//        {
//            return std::log(static_cast<float>(mWindowSize)) / std::log(2.0f) - 3.f;
//        }
//        if(identifier == mIdentifierWindowOverlapping)
//        {
//            anlWeakAssert(wrapper->getWrapper<PluginBufferingAdapter>() == nullptr);
//            if(wrapper->getWrapper<PluginBufferingAdapter>() != nullptr)
//            {
//                return getParameter("z2");
//            }
//            return std::log(static_cast<float>(mWindowOverlapping)) / std::log(2.0f);
//        }
//    }
//
//    return m_plugin->getParameter(identifier);
//}

//std::map<juce::String, double> Analyzer::Processor::getParameters() const
//{
//    std::map<juce::String, double> parameters;
//    auto const descriptors = getParameterDescriptors();
//    for(auto const& descriptor : descriptors)
//    {
//        parameters[descriptor.identifier] = getParameter(descriptor.identifier);
//    }
//    return parameters;
//}

bool Analyzer::Processor::prepareForAnalysis(juce::AudioFormatReader& audioFormatReader, std::map<juce::String, double> const& parameters)
{
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const windowSize = mWindowSize;
    auto const stepSize = windowSize / mWindowOverlapping;
    
    if(mPlugin == nullptr)
    {
        return false;
    }
    
    using PluginInputDomainAdapter = Vamp::HostExt::PluginInputDomainAdapter;
    auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(mPlugin.get());
    
    auto setParameterValue = [&](std::string identifier, float value)
    {
        if(wrapper != nullptr)
        {
            if(auto* adapter = wrapper->getWrapper<PluginInputDomainAdapter>())
            {
                if(identifier == mIdentifierWindowType)
                {
                    using WindowType = Vamp::HostExt::PluginInputDomainAdapter::WindowType;
                    adapter->setWindowType(static_cast<WindowType>(std::floor(value)));
                    return;
                }
                if(identifier == mIdentifierWindowSize)
                {
                    anlWeakAssert(value >= 0.0f && value <= 13.0f);
                    value = std::min(std::max(0.0f, value), 13.0f);
                    mWindowSize = static_cast<size_t>(std::floor(std::pow(2.0f, (value + 3.f))));
                    return;
                }
                if(identifier == mIdentifierWindowOverlapping)
                {
                    anlWeakAssert(value >= 0.0f && value <= 8.0f);
                    value = std::min(std::max(0.0f, value), 8.0f);
                    mWindowOverlapping = static_cast<size_t>(std::floor(std::pow(2.0f, (value))));
                    return;
                }
            }
        }
        mPlugin->setParameter(identifier, value);
    };
    
    for(auto const& parameter : parameters)
    {
        setParameterValue(parameter.first.toStdString(), static_cast<float>(parameter.second));
    }
    
    if(!mPlugin->initialise(static_cast<size_t>(numChannels), stepSize, windowSize))
    {
        return false;
    }
    mBuffer.setSize(numChannels, static_cast<int>(windowSize));
    mPosition = 0;
    
    mAudioFormatReader = &audioFormatReader;
    return true;
}

bool Analyzer::Processor::performNextAudioBlock(std::vector<Result>& results)
{
    anlStrongAssert(mPlugin != nullptr);
    anlStrongAssert(mAudioFormatReader != nullptr);
    if(mPlugin == nullptr || mAudioFormatReader == nullptr)
    {
        return false;
    }
    
    auto const feature = mFeature;
    auto const numChannels = static_cast<int>(mAudioFormatReader->numChannels);
    auto const lengthInSamples = mAudioFormatReader->lengthInSamples;
    auto const sampleRate = mAudioFormatReader->sampleRate;
    auto const windowSize = mWindowSize;
    auto const stepSize = windowSize / mWindowOverlapping;
    
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
    auto const remaininSamples = std::min(lengthInSamples - position, static_cast<juce::int64>(windowSize));
    
    mAudioFormatReader->read(writePointers, numChannels, position, static_cast<int>(remaininSamples));
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

std::unique_ptr<Analyzer::Processor> Analyzer::Processor::create(Accessor const& accessor, double sampleRate, AlertType alertType)
{
    JUCE_COMPILER_WARNING("Move this to plugin")
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto const key = accessor.getAttr<AttrType::key>();
    
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
        instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(sampleRate), PluginLoader::ADAPT_ALL_SAFE));
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

    return std::unique_ptr<Processor>(new Processor(std::move(instance), static_cast<size_t>(std::distance(outputs.cbegin(), feature))));
}

ANALYSE_FILE_END
