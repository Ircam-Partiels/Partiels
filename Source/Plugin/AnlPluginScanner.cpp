
#include "AnlPluginScanner.h"
#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::vector<std::string> Plugin::Scanner::getPluginKeys()
{
    auto* pluginLoader = Vamp::HostExt::PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {};
    }
    return pluginLoader->listPlugins();
}

void Plugin::Scanner::scan()
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    auto const paths = PluginHostAdapter::getPluginPath();
    for(auto const& path : paths)
    {
        DBG(path);
    }
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return;
    }
    auto const pluginList = pluginLoader->listPlugins();
    for(auto const& key : pluginList)
    {
        auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, 48000));
        if (plugin != nullptr)
        {
            auto const categories = pluginLoader->getPluginCategory(key);
            
            std::cout << "  --------\n";
            std::cout << "key: " << key << "\n";
            std::cout << "name: " << plugin->getName() << "\n";
            std::cout << "identifier: " << plugin->getIdentifier() << "\n";
            std::cout << "maker: " << plugin->getMaker() << "\n";
            std::cout << "api: " << plugin->getVampApiVersion() << "\n";
            std::cout << "cartegory:";
            for(auto const& category : categories)
            {
                std::cout << " " << category;
            }
            std::cout << "\n";
            std::cout << "desciption: " << plugin->getDescription() << "\n";
        }
    
    }
}

ANALYSE_FILE_END
