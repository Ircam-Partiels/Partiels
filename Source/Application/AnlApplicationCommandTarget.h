#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Plugin/AnlPluginListTable.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class CommandTarget
    : public juce::ApplicationCommandTarget
    , private juce::ChangeListener
    {
    public:
        // clang-format off
        enum CommandIDs : int
        {
              DocumentNew = 0x2001
            , DocumentOpen
            , DocumentSave
            , DocumentDuplicate
            , DocumentConsolidate
            
            , EditUndo
            , EditRedo
            , EditNewGroup
            , EditNewTrack
            , EditRemoveItem
            , EditLoadTemplate
            
            , TransportTogglePlayback
            , TransportToggleLooping
            , TransportRewindPlayHead
            
            , ZoomIn
            , ZoomOut
            
            , HelpOpenAudioSettings
            , HelpOpenAbout
            , HelpOpenManual
            , HelpOpenForum
        };
        // clang-format on

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
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
    };
} // namespace Application

ANALYSE_FILE_END
