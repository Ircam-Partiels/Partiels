#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class CommandTarget
    : public juce::ApplicationCommandTarget
    {
    public:
        
        enum CommandIDs : int
        {
            Open = 0x200100,
            New = 0x200101,
            Save = 0x200102,
            Duplicate = 0x200103,
            Consolidate = 0x200104,
            Close = 0x200105,
            OpenRecent
        };
        
        CommandTarget();
        ~CommandTarget() override = default;
        
        // juce::ApplicationCommandTarget
        juce::ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

    private:
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandTarget)
    };
}

ANALYSE_FILE_END

