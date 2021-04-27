#include "AnlPluginListScanner.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

Vamp::Plugin* PluginList::Scanner::loadPlugin(std::string const& key, float sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    std::unique_lock<std::mutex> lock(mMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        throw std::logic_error("plugin loader thread is already in used");
    }

    auto const entry = std::make_tuple(key, sampleRate);
    auto it = mPlugins.find(entry);
    if(it != mPlugins.end())
    {
        return it->second.get();
    }

    auto const pluginKeys = pluginLoader->listPlugins();
    if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), key) == pluginKeys.cend())
    {
        throw std::runtime_error("plugin key cannot be found");
    }

    auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(sampleRate), Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE));
    if(plugin == nullptr)
    {
        throw std::runtime_error("plugin allocation failed");
    }

    auto* pointer = plugin.get();
    mPlugins[entry] = std::move(plugin);
    return pointer;
}

std::tuple<std::set<Plugin::Key>, juce::StringArray> PluginList::Scanner::getKeys(double sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    std::set<Plugin::Key> keys;
    juce::StringArray errors;
    auto const pluginKeys = pluginLoader->listPlugins();
    for(auto const& pluginKey : pluginKeys)
    {
        try
        {
            auto* plugin = loadPlugin(pluginKey, static_cast<float>(sampleRate));
            if(plugin != nullptr)
            {
                auto const outputs = plugin->getOutputDescriptors();
                for(size_t feature = 0; feature < outputs.size(); ++feature)
                {
                    if(!keys.insert({pluginKey, outputs[feature].identifier}).second)
                    {
                        anlWeakAssert(false);
                        errors.add(pluginKey + ": insertion failed");
                    }
                }
            }
        }
        catch(std::exception& e)
        {
            errors.add(pluginKey + ": " + e.what());
        }
        catch(...)
        {
            errors.add(pluginKey + ": unknown error");
        }
    }
    return {keys, errors};
}

Plugin::Description PluginList::Scanner::getDescription(Plugin::Key const& key, double sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    auto* plugin = loadPlugin(key.identifier, static_cast<float>(sampleRate));
    if(plugin == nullptr)
    {
        throw std::runtime_error("allocation failed");
    }

    auto const outputs = plugin->getOutputDescriptors();
    for(size_t feature = 0; feature < outputs.size(); ++feature)
    {
        if(outputs[feature].identifier == key.feature)
        {
            Plugin::Description description;
            description.name = plugin->getName();
            description.inputDomain = Vamp::Plugin::InputDomain::TimeDomain;
            if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(plugin))
            {
                if(auto* inputDomainAdapter = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
                {
                    description.inputDomain = Vamp::Plugin::InputDomain::FrequencyDomain;
                    description.defaultState.windowType = inputDomainAdapter->getWindowType();
                }
            }

            description.maker = plugin->getMaker();
            description.version = static_cast<unsigned int>(plugin->getPluginVersion());
            auto const categories = pluginLoader->getPluginCategory(key.identifier);
            description.category = categories.empty() ? "" : categories.front();
            description.details = plugin->getDescription();

            auto const blockSize = plugin->getPreferredBlockSize();
            description.defaultState.blockSize = blockSize > 0 ? blockSize : 512;
            auto const stepSize = plugin->getPreferredStepSize();
            description.defaultState.stepSize = stepSize > 0 ? stepSize : description.defaultState.blockSize;
            auto const parameters = plugin->getParameterDescriptors();
            description.parameters.insert(description.parameters.cbegin(), parameters.cbegin(), parameters.cend());
            for(auto const& parameter : parameters)
            {
                description.defaultState.parameters[parameter.identifier] = parameter.defaultValue;
            }

            description.output = outputs[feature];
            return description;
        }
    }
    throw std::runtime_error("plugin feature cannot be found");
}

Plugin::Description PluginList::Scanner::loadDescription(Plugin::Key const& key, double sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {};
    }

    auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(sampleRate), Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE));
    if(plugin == nullptr)
    {
        return {};
    }

    auto const outputs = plugin->getOutputDescriptors();
    for(size_t feature = 0; feature < outputs.size(); ++feature)
    {
        if(outputs[feature].identifier == key.feature)
        {
            Plugin::Description description;
            description.name = plugin->getName();
            description.inputDomain = Vamp::Plugin::InputDomain::TimeDomain;
            if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(plugin.get()))
            {
                if(auto* inputDomainAdapter = wrapper->getWrapper<Vamp::HostExt::PluginInputDomainAdapter>())
                {
                    description.inputDomain = Vamp::Plugin::InputDomain::FrequencyDomain;
                    description.defaultState.windowType = inputDomainAdapter->getWindowType();
                }
            }

            description.maker = plugin->getMaker();
            description.version = static_cast<unsigned int>(plugin->getPluginVersion());
            auto const categories = pluginLoader->getPluginCategory(key.identifier);
            description.category = categories.empty() ? "" : categories.front();
            description.details = plugin->getDescription();

            auto const blockSize = plugin->getPreferredBlockSize();
            description.defaultState.blockSize = blockSize > 0 ? blockSize : 512;
            auto const stepSize = plugin->getPreferredStepSize();
            description.defaultState.stepSize = stepSize > 0 ? stepSize : description.defaultState.blockSize;
            auto const parameters = plugin->getParameterDescriptors();
            description.parameters.insert(description.parameters.cbegin(), parameters.cbegin(), parameters.cend());
            for(auto const& parameter : parameters)
            {
                description.defaultState.parameters[parameter.identifier] = parameter.defaultValue;
            }

            description.output = outputs[feature];
            return description;
        }
    }
    return {};
}

ANALYSE_FILE_END
