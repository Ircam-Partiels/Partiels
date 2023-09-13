#include "AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Ive::PluginWrapper* PluginList::Scanner::loadPlugin(std::string const& key, float sampleRate)
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

    auto* plugin = pluginLoader->loadPlugin(key, static_cast<float>(sampleRate), Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE);
    if(plugin == nullptr)
    {
        throw std::runtime_error("plugin allocation failed");
    }
    auto wrapper = std::make_unique<Ive::PluginWrapper>(plugin, key);
    if(wrapper == nullptr)
    {
        throw std::runtime_error("plugin allocation failed");
    }

    auto* pointer = wrapper.get();
    mPlugins[entry] = std::move(wrapper);
    return pointer;
}

std::tuple<std::map<Plugin::Key, Plugin::Description>, juce::StringArray> PluginList::Scanner::getPlugins(double sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    std::map<Plugin::Key, Plugin::Description> list;
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
                    Plugin::Key const key{pluginKey, outputs[feature].identifier};
                    anlWeakAssert(list.count(key) == 0_z);
                    if(list.count(key) > 0_z)
                    {
                        errors.add(pluginKey + ": duplicate key");
                    }
                    else if(!list.insert({key, Plugin::loadDescription(*plugin, key)}).second)
                    {
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
    return {list, errors};
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
    return Plugin::loadDescription(*plugin, key);
}

Plugin::Description PluginList::Scanner::loadDescription(Plugin::Key const& key, double sampleRate)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {};
    }
    auto* plugin = pluginLoader->loadPlugin(key.identifier, static_cast<float>(sampleRate), Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE);
    if(plugin == nullptr)
    {
        return {};
    }
    auto wrapper = std::make_unique<Ive::PluginWrapper>(plugin, key.identifier);
    if(wrapper == nullptr)
    {
        return {};
    }
    return Plugin::loadDescription(*wrapper.get(), key);
}

ANALYSE_FILE_END
