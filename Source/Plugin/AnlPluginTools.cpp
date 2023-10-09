#include "AnlPluginTools.h"

ANALYSE_FILE_BEGIN

Plugin::LoadingError::LoadingError(char const* message)
: std::runtime_error(message)
{
}

Plugin::ParametersError::ParametersError(char const* message)
: std::runtime_error(message)
{
}

std::unique_ptr<juce::Component> Plugin::Tools::createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange)
{
    auto const name = juce::String(Format::withFirstCharUpperCase(parameter.name));
    if(!parameter.valueNames.empty())
    {
        return std::make_unique<PropertyList>(name, parameter.description, parameter.unit, parameter.valueNames, [=](size_t index)
                                              {
                                                  if(applyChange == nullptr)
                                                  {
                                                      return;
                                                  }
                                                  applyChange(parameter, static_cast<float>(index));
                                              });
    }
    else if(parameter.isQuantized && std::abs(parameter.quantizeStep - 1.0f) < std::numeric_limits<float>::epsilon() && std::abs(parameter.minValue) < std::numeric_limits<float>::epsilon() && std::abs(parameter.maxValue - 1.0f) < std::numeric_limits<float>::epsilon())
    {
        return std::make_unique<PropertyToggle>(name, parameter.description, [=](bool state)
                                                {
                                                    if(applyChange == nullptr)
                                                    {
                                                        return;
                                                    }
                                                    applyChange(parameter, state ? 1.0f : 0.f);
                                                });
    }

    auto const description = juce::String(parameter.description) + " [" + juce::String(parameter.minValue, 2) + ":" + juce::String(parameter.maxValue, 2) + (!parameter.isQuantized ? "" : ("-" + juce::String(parameter.quantizeStep, 2))) + "]";
    return std::make_unique<PropertyNumber>(name, description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? parameter.quantizeStep : 0.0f, [=](float value)
                                            {
                                                if(applyChange == nullptr)
                                                {
                                                    return;
                                                }
                                                applyChange(parameter, value);
                                            });
}

std::vector<std::unique_ptr<Ive::PluginWrapper>> Plugin::Tools::createPluginWrappers(Key const& key, State const& state, size_t numReaderChannels, double readerSampleRate)
{
    using namespace Vamp::HostExt;
    auto* pluginLoader = PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    anlStrongAssert(!key.identifier.empty());
    if(key.identifier.empty())
    {
        throw LoadingError("plugin key is not defined.");
    }
    anlStrongAssert(!key.feature.empty());
    if(key.feature.empty())
    {
        throw LoadingError("plugin feature is not defined.");
    }
    anlStrongAssert(state.blockSize > 0);
    if(state.blockSize == 0)
    {
        throw ParametersError("plugin block size is invalid");
    }
    anlStrongAssert(state.stepSize > 0);
    if(state.stepSize == 0)
    {
        throw ParametersError("plugin step size is invalid");
    }

    std::vector<std::unique_ptr<Ive::PluginWrapper>> plugins;
    auto const addAndInitializeInstance = [&](std::unique_ptr<Ive::PluginWrapper>& plugin, size_t numChannels)
    {
        MiscWeakAssert(plugin != nullptr);
        if(plugin == nullptr)
        {
            throw ParametersError("plugin invalid");
        }
        if(auto* adapter = plugin->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
        {
            adapter->setWindowType(state.windowType);
        }

        auto const descriptors = plugin->getParameterDescriptors();
        for(auto const& parameter : state.parameters)
        {
            if(std::any_of(descriptors.cbegin(), descriptors.cend(), [&](auto const& descriptor)
                           {
                               return descriptor.identifier == parameter.first;
                           }))
            {
                plugin->setParameter(parameter.first, parameter.second);
            }
        }

        if(!plugin->initialise(numChannels, state.stepSize, state.blockSize))
        {
            throw ParametersError("plugin initialization failed");
        }
        plugins.push_back(std::move(plugin));
    };

    auto const sampleRate = static_cast<float>(readerSampleRate);
    auto instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
    if(instance == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }
    auto wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
    if(wrapper == nullptr)
    {
        throw LoadingError("plugin library couldn't be loaded");
    }

    auto const maxChannels = wrapper->getMaxChannelCount();
    while(wrapper != nullptr)
    {
        auto const numChannels = std::min(maxChannels, numReaderChannels);
        numReaderChannels -= numChannels;
        addAndInitializeInstance(wrapper, numChannels);
        if(numReaderChannels > 0_z)
        {
            instance = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, sampleRate, PluginLoader::ADAPT_INPUT_DOMAIN));
            if(instance == nullptr)
            {
                throw LoadingError("plugin library couldn't be loaded");
            }
            wrapper = std::make_unique<Ive::PluginWrapper>(instance.release(), key.identifier);
            if(wrapper == nullptr)
            {
                throw LoadingError("plugin library couldn't be loaded");
            }
        }
    }
    return plugins;
}

ANALYSE_FILE_END
