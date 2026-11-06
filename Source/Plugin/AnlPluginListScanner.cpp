#include "AnlPluginListScanner.h"
#include "AnlPluginTools.h"

ANALYSE_FILE_BEGIN

Ive::PluginWrapper* PluginList::Scanner::loadPlugin(std::string const& key, float sampleRate)
{
    std::unique_lock<std::mutex> lock(mMutex);
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    auto const entry = std::make_tuple(key, sampleRate);
    auto it = mPlugins.find(entry);
    if(it != mPlugins.end())
    {
        return it->second.get();
    }

    auto const pluginKeys = pluginLoader->listPlugins(false);
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
    auto* pointer = wrapper.get();
    mPlugins[entry] = std::move(wrapper);
    return pointer;
}

std::tuple<std::map<Plugin::Key, Plugin::Description>, juce::StringArray> PluginList::Scanner::getPlugins(double sampleRate, bool forceRescan)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("plugin loader is not available");
    }

    std::unique_lock<std::mutex> lock(mMutex);
    if(forceRescan)
    {
        // Destroying the plugin instances releases the dynamic libraries held by the loader, otherwise the update of an already loaded plugin is ignored
        mPlugins.clear();
    }
    lock.unlock();
    std::map<Plugin::Key, Plugin::Description> list;
    juce::StringArray errors;
    auto const pluginKeys = pluginLoader->listPlugins(forceRescan);
    for(auto const& pluginKey : pluginKeys)
    {
        try
        {
            auto* plugin = loadPlugin(pluginKey, static_cast<float>(sampleRate));
            if(plugin != nullptr)
            {
                // The voices of an output are represented by the first voice
                auto const outputs = plugin->getOutputDescriptors();
                for(auto feature = 0_z; feature < outputs.size(); feature += std::clamp(plugin->getOutputVoiceCount(feature), 1_z, outputs.size() - feature))
                {
                    Plugin::Key const key{pluginKey, outputs[feature].identifier};
                    MiscWeakAssert(list.count(key) == 0_z);
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
    MiscStrongAssert(pluginLoader != nullptr);
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
    return Plugin::loadDescription(*wrapper.get(), key);
}

Plugin::Description PluginList::Scanner::loadDescription(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
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
    // The numbers of voices must be set before retrieving the outputs
    Plugin::Tools::setVoiceCounts(*wrapper.get(), voiceCounts);
    Plugin::Tools::setParameters(*wrapper.get(), state);
    return Plugin::loadDescription(*wrapper.get(), key);
}

std::vector<Plugin::Description> PluginList::Scanner::loadVoiceDescriptions(Plugin::Key const& key, double sampleRate, Plugin::State const& state, Plugin::VoiceCounts const& voiceCounts)
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    MiscStrongAssert(pluginLoader != nullptr);
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
    // The numbers of voices must be set before retrieving the outputs
    Plugin::Tools::setVoiceCounts(*wrapper.get(), voiceCounts);
    Plugin::Tools::setParameters(*wrapper.get(), state);

    auto const featureIndex = Plugin::Tools::getFeatureIndex(*wrapper.get(), key.feature);
    if(!featureIndex.has_value())
    {
        return {};
    }
    auto voices = Plugin::getVoices(*wrapper.get(), featureIndex.value());
    if(voices.empty())
    {
        voices.push_back(key.feature);
    }
    std::vector<Plugin::Description> descriptions;
    descriptions.reserve(voices.size());
    for(auto const& voice : voices)
    {
        descriptions.push_back(Plugin::loadDescription(*wrapper.get(), {key.identifier, voice}));
    }
    return descriptions;
}

ANALYSE_FILE_END
