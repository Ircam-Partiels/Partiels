
#include "AnlPluginListScanner.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::map<Plugin::Key, Plugin::Description> PluginList::Scanner::getPluginDescriptions(double defaultSampleRate)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        throw std::runtime_error("Cannot get the plugin loader");
    }
    
    std::map<Plugin::Key, Plugin::Description> descriptions;
    auto const keys = pluginLoader->listPlugins();
    for(auto const& key : keys)
    {
        auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(defaultSampleRate)));
        if(plugin != nullptr)
        {
            Plugin::Description common;
            common.name = plugin->getName();
            common.maker = plugin->getMaker();
            common.api = plugin->getVampApiVersion();
            auto const categories = pluginLoader->getPluginCategory(key);
            common.categories = {categories.cbegin(), categories.cend()};
            common.details = plugin->getDescription();
            auto const outputs = plugin->getOutputDescriptors();
            for(size_t feature = 0; feature < outputs.size(); ++feature)
            {
                Plugin::Description description(common);
                description.specialization = outputs[feature].name;
                descriptions[{key, outputs[feature].identifier}] = description;
            }
        }
    }
    return descriptions;
}

ANALYSE_FILE_END
