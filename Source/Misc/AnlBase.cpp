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

bool Utils::isCommandTicked(juce::ApplicationCommandManager& commandManager, int command)
{
    juce::ApplicationCommandInfo commandInfo(0);
    if(commandManager.getTargetForCommand(command, commandInfo) != nullptr)
    {
        return commandInfo.flags & juce::ApplicationCommandInfo::CommandFlags::isTicked;
    }
    return false;
}

juce::String Utils::getCommandDescriptionWithKey(juce::ApplicationCommandManager const& commandManager, int command)
{
    if(auto const* ci = commandManager.getCommandForID(command))
    {
        auto description = ci->description.isNotEmpty() ? ci->description : ci->shortName;
        for(auto& eypresses : ci->defaultKeypresses)
        {
            auto const key = eypresses.getTextDescription();
            description << " [";
            if(key.length() == 1)
            {
                description << juce::translate("shortcut") << ": '" << key << "']";
            }
            else
            {
                description << key << ']';
            }
        }
        return description;
    }
    return {};
}

ANALYSE_FILE_END
