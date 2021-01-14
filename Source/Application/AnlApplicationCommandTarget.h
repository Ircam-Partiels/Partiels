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
              DocumentOpen = 0x2001
            , DocumentSave
            , DocumentDuplicate
            , DocumentConsolidate
            , DocumentOpenTemplate
            , DocumentSaveTemplate
            
            , EditUndo
            , EditRedo
            
            , AnalysisOpen
            , AnalysisNew
            , AnalysisSave
            , AnalysisDuplicate
            , AnalysisRemove
            , AnalysisProperties
            , AnalysisExport
            
            , PointsNew
            , PointsRemove
            , PointsMove
            , PointsCopy
            , PointsPaste
            , PointsScale
            , PointsQuantify
            , PointsDiscretize
            
            , TransportTogglePlayback
            , TransportToggleLooping
            , TransportRewindPlayHead
            
            , ZoomIn
            , ZoomOut
            
            , HelpOpenManual
            , HelpOpenForum

            , DocumentOpenRecent
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

