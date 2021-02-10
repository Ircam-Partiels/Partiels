
#include "AnlPluginListScanner.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

std::set<Plugin::Key> PluginList::Scanner::getPluginKeys(double sampleRate, AlertType const alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Enumeration of plugins failed!"), juce::translate("Cannot get the plugin loader."));
        }
        return {};
    }
    
    auto loadPlugin = [&](std::string const& key) -> std::unique_ptr<Vamp::Plugin>
    {
        try
        {
            auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(sampleRate), PluginLoader::ADAPT_ALL_SAFE));
        }
        catch(std::exception const& e)
        {
            if(alertType == AlertType::window)
            {
                using AlertIconType = juce::AlertWindow::AlertIconType;
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Enumeration of plugins failed!"), juce::translate(e.what()));
            }
        }
        return nullptr;
    };
    
    std::set<Plugin::Key> keys;
    auto const pluginKeys = pluginLoader->listPlugins();
    for(auto const& pluginKey : pluginKeys)
    {
        auto plugin = loadPlugin(pluginKey);
        if(plugin != nullptr)
        {
            auto const outputs = plugin->getOutputDescriptors();
            for(size_t feature = 0; feature < outputs.size(); ++feature)
            {
                if(!keys.insert({pluginKey, outputs[feature].identifier}).second)
                {
                    anlWeakAssert(false);
                }
            }
        }
    }
    return keys;
}

Plugin::Description PluginList::Scanner::getPluginDescription(Plugin::Key const& key, double sampleRate, AlertType const alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugins failed!"), juce::translate("Cannot get the plugin loader."));
        }
        return {};
    }
    
    std::unique_ptr<Vamp::Plugin> plugin;
    try
    {
        plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key.identifier, static_cast<float>(sampleRate), PluginLoader::ADAPT_ALL_SAFE));
    }
    catch(std::exception const& e)
    {
        if(alertType == AlertType::window)
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugins failed!"), juce::translate(e.what()));
        }
        return {};
    }
    
    if(plugin == nullptr)
    {
        if(alertType == AlertType::window)
        {
            using AlertIconType = juce::AlertWindow::AlertIconType;
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugins failed!"), juce::translate("Plugin \"PLGKEY\" cannot be found.").replace("PLGKEY", key.identifier));
        }
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
                if(auto* inputDomainAdapter = wrapper->getWrapper<PluginInputDomainAdapter>())
                {
                    description.inputDomain = Vamp::Plugin::InputDomain::FrequencyDomain;
                    description.defaultState.windowType = inputDomainAdapter->getWindowType();
                }
            }
            
            description.maker = plugin->getMaker();
            description.version = static_cast<unsigned int>(plugin->getPluginVersion());
            auto const categories = pluginLoader->getPluginCategory(key.identifier);
            description.category = categories.empty() ? "": categories.front();
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
    
    if(alertType == AlertType::window)
    {
        using AlertIconType = juce::AlertWindow::AlertIconType;
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugins failed!"), juce::translate("Feature \"FTRID\" of plugin \"PLGKEY\" cannot be found.").replace("FTRID", key.feature).replace("PLGKEY", key.identifier));
    }
    return {};
}

ANALYSE_FILE_END
