#include "AnlAnalyzerPluginManager.h"
#include "../Plugin/AnlPluginListScanner.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

std::shared_ptr<Vamp::Plugin> Analyzer::PluginManager::createInstance(Accessor const& accessor, bool showMessageOnFailure)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto const pluginKey = accessor.getValue<AttrType::key>();
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
        if(showMessageOnFailure)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin manager is not available.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    auto const sampleRate = static_cast<int>(accessor.getValue<AttrType::sampleRate>());
    auto pluginInstance = std::shared_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), sampleRate, PluginLoader::ADAPT_ALL));
    if(pluginInstance == nullptr)
    {
        if(showMessageOnFailure)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
        }
        return nullptr;
    }
    
    return pluginInstance;
}

ANALYSE_FILE_END
