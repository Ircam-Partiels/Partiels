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
            Open = 0x200100
            , New
            , Save
            , Duplicate
            , Consolidate
            
            , TogglePlayback
            , ToggleLooping
            , MovePlayHeadToBeginning
            , MovePlayHeadToEnd
            
            , AddAnalysis
            
            , OpenRecent
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

