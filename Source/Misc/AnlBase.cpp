#include "AnlBase.h"

ANALYSE_FILE_BEGIN

void Utils::notifyListener(juce::ApplicationCommandManager& commandManager, juce::ApplicationCommandManagerListener& listener, std::vector<int> const& commandIds)
{
    for(auto const commandId : commandIds)
    {
        juce::ApplicationCommandInfo commandInfo(0);
        if(commandManager.getTargetForCommand(commandId, commandInfo) != nullptr)
        {
            juce::ApplicationCommandTarget::InvocationInfo info(commandId);
            info.commandFlags = commandInfo.flags;
            listener.applicationCommandInvoked(info);
        }
    }
}

ANALYSE_FILE_END
