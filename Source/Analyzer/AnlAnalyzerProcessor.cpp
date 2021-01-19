#include "AnlAnalyzerProcessor.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginWrapper.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginBufferingAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::Processor::Processor(Vamp::Plugin* plugin, size_t feature)
: Vamp::HostExt::PluginWrapper(plugin)
, mFeature(feature)
{
    auto const& descriptors = m_plugin->getParameterDescriptors();
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

size_t Analyzer::Processor::getWindowSize() const
{
    return mWindowSize;
}

size_t Analyzer::Processor::getStepSize() const
{
    return mWindowSize / mWindowOverlapping;
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
            descriptor.identifier = mIdentifierWindowOverlapping;
            descriptor.name = "Window Overlapping";
            descriptor.description = "The window overlapping used to perform the FFT";
            descriptor.unit = "x";
            for(unsigned int i = 1; i <= 16; i *= 2)
            {
                descriptor.valueNames.push_back(std::to_string(i));
            }
            descriptor.minValue = 0.0f;
            descriptor.minValue = static_cast<float>(descriptor.valueNames.size() - 1);
            descriptor.defaultValue = 4.0f;
            descriptor.isQuantized = true;
            descriptor.quantizeStep = 1.0f;
            parameters.insert(parameters.begin(), descriptor);
        }

        {
            Vamp::Plugin::ParameterDescriptor descriptor;
            descriptor.identifier = mIdentifierWindowSize;
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
            descriptor.identifier = mIdentifierWindowType;
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
        if(identifier == mIdentifierWindowType)
        {
            return static_cast<float>(adapter->getWindowType());
        }
        if(identifier == mIdentifierWindowSize)
        {
            return std::log(static_cast<float>(mWindowSize)) / std::log(2.0f) - 3.f;
        }
        if(identifier == mIdentifierWindowOverlapping)
        {
            anlWeakAssert(wrapper->getWrapper<PluginBufferingAdapter>() == nullptr);
            if(wrapper->getWrapper<PluginBufferingAdapter>() != nullptr)
            {
                return getParameter("z2");
            }
            return std::log(static_cast<float>(mWindowOverlapping)) / std::log(2.0f);
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
                anlWeakAssert(adapter->getWrapper<PluginBufferingAdapter>() == nullptr);
                if(adapter->getWrapper<PluginBufferingAdapter>() != nullptr)
                {
                    return;
                }
                mWindowOverlapping = static_cast<size_t>(std::floor(std::pow(2.0f, (value))));
                return;
            }
        }
    }
    
    
    m_plugin->setParameter(identifier, value);
}

std::map<juce::String, double> Analyzer::Processor::getParameters() const
{
    std::map<juce::String, double> parameters;
    auto const descriptors = getParameterDescriptors();
    for(auto const& descriptor : descriptors)
    {
        parameters[descriptor.identifier] = getParameter(descriptor.identifier);
    }
    return parameters;
}

void Analyzer::Processor::setParameters(std::map<juce::String, double> const& parameters)
{
    for(auto const& parameter : parameters)
    {
        setParameter(parameter.first.toStdString(), static_cast<float>(parameter.second));
    }
}

bool Analyzer::Processor::hasZoomInfo(size_t const feature) const
{
    auto const descriptors = getOutputDescriptors();
    anlWeakAssert(feature < descriptors.size());
    if(feature >= descriptors.size())
    {
        return false;
    }
    return descriptors[feature].hasKnownExtents;
}

std::tuple<Zoom::Range, double> Analyzer::Processor::getZoomInfo(size_t const feature) const
{
    if(!hasZoomInfo(feature))
    {
        return std::make_tuple(Zoom::Range{0.0, 0.0}, 0.0);
    }
    auto const& descriptor = getOutputDescriptors()[feature];
    return std::make_tuple(Zoom::Range{descriptor.minValue, descriptor.maxValue}, descriptor.isQuantized ? descriptor.quantizeStep : 0.0);
}

bool Analyzer::Processor::prepareForAnalysis(juce::AudioFormatReader& audioFormatReader)
{
    auto const numChannels = static_cast<int>(audioFormatReader.numChannels);
    auto const windowSize = getWindowSize();
    auto const stepSize = getStepSize();
    
    if(!initialise(static_cast<size_t>(numChannels), stepSize, windowSize))
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
    anlStrongAssert(mAudioFormatReader != nullptr);
    anlStrongAssert(mFeature < getOutputDescriptors().size());
    auto const feature = mFeature;
    if(mAudioFormatReader == nullptr || feature >= getOutputDescriptors().size())
    {
        return false;
    }
    
    auto const numChannels = static_cast<int>(mAudioFormatReader->numChannels);
    auto const lengthInSamples = mAudioFormatReader->lengthInSamples;
    auto const sampleRate = mAudioFormatReader->sampleRate;
    auto const windowSize = getWindowSize();
    auto const stepSize = getStepSize();
    
    auto** writePointers = mBuffer.getArrayOfWritePointers();
    auto const* const* readPointers = mBuffer.getArrayOfReadPointers();
    auto const position = mPosition;
    mPosition = position + static_cast<juce::int64>(stepSize);
    
    if(position > lengthInSamples)
    {
        auto result = getRemainingFeatures();
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
    
    auto result = process(readPointers, rt);
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

std::unique_ptr<Analyzer::Processor> Analyzer::createProcessor(Accessor const& accessor, double sampleRate, AlertType alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto const key = accessor.getAttr<AttrType::key>();
    if(key.identifier.empty() || key.feature.empty())
    {
        return nullptr;
    }
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    
    using AlertIconType = juce::AlertWindow::AlertIconType;
    auto const errorTitle = juce::translate("Plugin cannot be loaded!");
    auto const errorMessage =  juce::translate("The plugin \"PLGNKEY: FTRKEY\" cannot be loaded: ").replace("PLGNKEY", key.identifier).replace("FTRKEY", key.feature);
    
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
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, errorTitle, errorMessage + juce::translate("Invalid feature key") + ".");
        }
        return nullptr;
    }
    
    return std::make_unique<Processor>(instance.release(), std::distance(outputs.cbegin(), feature));
}

ANALYSE_FILE_END
