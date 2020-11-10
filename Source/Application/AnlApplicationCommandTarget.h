#pragma once

#include "AnlApplicationModel.h"
#include "../Document/AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class CommandTarget
    : public juce::ApplicationCommandTarget
    , private juce::ChangeListener
    {
    public:

        enum CommandIDs : int
        {
            Open = 0x200100,
            New = 0x200101,
            Save = 0x200102,
            Duplicate = 0x200103,
            Consolidate = 0x200104,
            
            TogglePlayback = 0x200105,
            ToggleLooping = 0x200106,
            MovePlayHeadToBeginning = 0x200107,
            MovePlayHeadToEnd = 0x200108,
            
            OpenRecent = 0x200200 // Always at the end
        };
        
        CommandTarget();
        ~CommandTarget() override;
        
        // juce::ApplicationCommandTarget
        juce::ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

    private:
        
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        Accessor::Listener mListener;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandTarget)
    };
}

ANALYSE_FILE_END

