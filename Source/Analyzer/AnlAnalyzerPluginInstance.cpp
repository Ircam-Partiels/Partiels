#include "AnlAnalyzerPluginInstance.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Analyzer::PluginInstance::PluginInstance(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        if(attribute == Attribute::key)
        {
            using namespace Vamp;
            using namespace Vamp::HostExt;
            
            auto const pluginKey = acsr.getModel().key;
            if(pluginKey.isEmpty())
            {
                mAccessor.setInstance({});
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, juce::NotificationType::sendNotificationSync);
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
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, juce::NotificationType::sendNotificationSync);
                return;
            }
            
            auto pluginInstance = std::shared_ptr<Vamp::Plugin>(pluginLoader->loadPlugin(pluginKey.toStdString(), 48000, PluginLoader::ADAPT_ALL));
            if(pluginInstance == nullptr)
            {
                juce::AlertWindow::showMessageBox(AlertIconType::WarningIcon, errorMessage, juce::translate("The plugin PLGNKEY cannot be loaded because the plugin key is invalid.").replace("PLGNKEY", pluginKey));
                mAccessor.setInstance({});
                mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, juce::NotificationType::sendNotificationSync);
                return;
            }
            mAccessor.setInstance(pluginInstance);
            mAccessor.sendSignal(Signal::pluginInstanceChanged, {}, juce::NotificationType::sendNotificationSync);
        }
        
    };
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
}

Analyzer::PluginInstance::~PluginInstance()
{
    mAccessor.removeListener(mListener);
}

ANALYSE_FILE_END
