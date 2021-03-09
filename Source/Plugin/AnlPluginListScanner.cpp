#include "AnlPluginListScanner.h"
#include <vamp-hostsdk/PluginLoader.h>

ANALYSE_FILE_BEGIN

std::tuple<Vamp::Plugin*, std::optional<juce::String>> PluginList::Scanner::loadPlugin(std::string const& key, float sampleRate)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        return {nullptr, "Cannot get the plugin loader."};
    }
    
    std::unique_lock<std::mutex> lock(mMutex, std::try_to_lock);
    anlStrongAssert(lock.owns_lock());
    if(!lock.owns_lock())
    {
        return {nullptr, "Cannot get the plugin loader thread."};
    }
    
    auto const entry = std::make_tuple(key, sampleRate);
    if(mPlugins.count(entry) > 0_z)
    {
        return {mPlugins.at(entry).get(), {}};
    }
    
    auto const pluginKeys = pluginLoader->listPlugins();
    if(std::find(pluginKeys.cbegin(), pluginKeys.cend(), key) == pluginKeys.cend())
    {
        return {nullptr, juce::translate("Plugin \"PLGKEY\" cannot be found.").replace("PLGKEY", key)};
    }
    
    try
    {
        auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(sampleRate), PluginLoader::ADAPT_ALL_SAFE));
        if(plugin != nullptr)
        {
            mPlugins[entry] = std::move(plugin);
            return {mPlugins[entry].get(), {}};
        }
    }
    catch(std::exception const& e)
    {
        return {nullptr, juce::translate(e.what()).toStdString()};
    }
    catch(...)
    {
        return {nullptr, juce::translate("Unknown reason")};
    }
    return {nullptr, juce::translate("Cannot allocate the plugin.")};
}

std::set<Plugin::Key> PluginList::Scanner::getPluginKeys(double sampleRate, AlertType const alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    using AlertIconType = juce::AlertWindow::AlertIconType;
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Enumeration of plugins failed!"), juce::translate("Cannot get the plugin loader."));
        }
        return {};
    }

    juce::StringArray errors;
    std::set<Plugin::Key> keys;
    auto const pluginKeys = pluginLoader->listPlugins();
    for(auto const& pluginKey : pluginKeys)
    {
        auto result = loadPlugin(pluginKey, static_cast<float>(sampleRate));
        if(std::get<0>(result) != nullptr)
        {
            auto const outputs = std::get<0>(result)->getOutputDescriptors();
            for(size_t feature = 0; feature < outputs.size(); ++feature)
            {
                if(!keys.insert({pluginKey, outputs[feature].identifier}).second)
                {
                    anlWeakAssert(false);
                }
            }
        }
        if(std::get<1>(result))
        {
            errors.add(*std::get<1>(result));
        }
    }
    
    if(!errors.isEmpty() && alertType == AlertType::window)
    {
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Enumeration of plugins failed!"), errors.joinIntoString("\n"));
    }
    
    return keys;
}

Plugin::Description PluginList::Scanner::getPluginDescription(Plugin::Key const& key, double sampleRate, AlertType const alertType)
{
    using namespace Vamp;
    using namespace Vamp::HostExt;
    using AlertIconType = juce::AlertWindow::AlertIconType;
    
    auto* pluginLoader = PluginLoader::getInstance();
    anlWeakAssert(pluginLoader != nullptr);
    if(pluginLoader == nullptr)
    {
        if(alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugin failed!"), juce::translate("Cannot get the plugin loader."));
        }
        return {};
    }
    
    auto result = loadPlugin(key.identifier, static_cast<float>(sampleRate));
    auto* plugin = std::get<0>(result);
    if(plugin != nullptr)
    {
        if(std::get<1>(result) && alertType == AlertType::window)
        {
            juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugin failed!"), *std::get<1>(result));
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
            if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(plugin))
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
        juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, juce::translate("Loading of plugin failed!"), juce::translate("Feature \"FTRID\" of plugin \"PLGKEY\" cannot be found.").replace("FTRID", key.feature).replace("PLGKEY", key.identifier));
    }
    return {};
}

ANALYSE_FILE_END
