#pragma once

#include "AnlApplicationModel.h"
#include "../Document/AnlDocumentFileBased.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Plugin/AnlPluginListScanner.h"

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
            , RewindPlayHead
            
            , AnalysisAdd
            , AnalysisDuplicate
            , AnalysisRemove
            , AnalysisProperties
            
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
        
        JUCE_DEPRECATED(static void showUnsupportedAction());
        
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        Accessor::Listener mListener;
  
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandTarget)
    };
    
    class MainMenuModel
    : public juce::MenuBarModel
    {
    public:
        MainMenuModel() = default;
        ~MainMenuModel() override = default;
        
        juce::StringArray getMenuBarNames() override;
        juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName) override;
        void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    };
}

ANALYSE_FILE_END

