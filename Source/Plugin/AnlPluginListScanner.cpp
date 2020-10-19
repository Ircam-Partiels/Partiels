
#include "AnlPluginListScanner.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::set<std::string> PluginList::Scanner::getPluginKeys()
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("Cannot get the plugin loader");
    }
    auto keys = pluginLoader->listPlugins();
    return {keys.cbegin(), keys.cend()};
}

std::map<juce::String, PluginList::Model::Description> PluginList::Scanner::getPluginDescriptions(double defaultSampleRate)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("Cannot get the plugin loader");
    }
    
    std::map<juce::String, Description> descriptions;
    auto const keys = getPluginKeys();
    for(auto const& key : keys)
    {
        Description description;
        auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(defaultSampleRate)));
        if (plugin != nullptr)
        {
            description.name = plugin->getName();
            description.maker = plugin->getMaker();
            description.api = plugin->getVampApiVersion();
            auto const categories = pluginLoader->getPluginCategory(key);
            description.categories = {categories.cbegin(), categories.cend()};
            description.details = plugin->getDescription();
        }
        descriptions[key] = description;
    }
    return descriptions;
}

ANALYSE_FILE_END
