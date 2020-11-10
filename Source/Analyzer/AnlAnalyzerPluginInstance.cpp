#include "AnlAnalyzerPluginInstance.h"
#include "../Plugin/AnlPluginListScanner.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

Analyzer::PluginInstance::PluginInstance(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::key)
        {
            using namespace Vamp;
            using namespace Vamp::HostExt;
            
            auto const pluginKey = acsr.getValue<AttrType::key>();
            if(pluginKey.isEmpty())
            {
                mAccessor.setInstance({});
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, NotificationType::synchronous);
                return;
            }
            auto* pluginLoader = PluginLoader::getInstance();
            anlWeakAssert(pluginLoader != nullptr);
            
            using AlertIconType = juce::AlertWindow::AlertIconType;
            auto const errorMessage = juce::translate("Plugin cannot be loaded!");
            
            if(pluginLoader == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin manager is not available.").replace("PLGNKEY", pluginKey));
                mAccessor.setInstance({});
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, NotificationType::synchronous);
                return;
            }
            
            auto pluginInstance = std::shared_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), 48000, PluginLoader::ADAPT_ALL));
            if(pluginInstance == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
                mAccessor.setInstance({});
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, NotificationType::synchronous);
                return;
            }
            mAccessor.setInstance(pluginInstance);
            mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, NotificationType::synchronous);
            JUCE_COMPILER_WARNING("todo");
//            if(mAccessor.getModel().name.isEmpty())
//            {
//                auto copy = mAccessor.getModel();
//                copy.name = pluginInstance->getName();
//                mAccessor.fromModel(copy, NotificationType::asynchronous);
//            }
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::PluginInstance::~PluginInstance()
{
    mAccessor.removeListener(mListener);
}

ANALYSE_FILE_END
