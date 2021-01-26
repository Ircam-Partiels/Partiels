
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
        auto plugin = std::unique_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(key, static_cast<float>(defaultSampleRate), PluginLoader::ADAPT_ALL_SAFE));
        if(plugin != nullptr)
        {
            Plugin::Description common;
            common.name = plugin->getName();
            common.inputDomain = Vamp::Plugin::InputDomain::TimeDomain;
            if(auto* wrapper = dynamic_cast<Vamp::HostExt::PluginWrapper*>(plugin.get()))
            {
                common.inputDomain = wrapper->getWrapper<PluginInputDomainAdapter>() != nullptr ? Vamp::Plugin::InputDomain::FrequencyDomain : Vamp::Plugin::InputDomain::TimeDomain;
            }
            
            common.maker = plugin->getMaker();
            common.version = static_cast<unsigned int>(plugin->getPluginVersion());
            auto const categories = pluginLoader->getPluginCategory(key);
            common.category = categories.empty() ? "": categories.front();
            common.details = plugin->getDescription();
        
            auto const blockSize = plugin->getPreferredBlockSize();
            common.defaultBlockSize = blockSize > 0 ? blockSize : 512;
            auto const stepSize = plugin->getPreferredStepSize();
            common.defaultStepSize = stepSize > 0 ? stepSize : blockSize;
            auto const parameters = plugin->getParameterDescriptors();
            common.parameters.insert(common.parameters.cbegin(), parameters.cbegin(), parameters.cend());
            
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
