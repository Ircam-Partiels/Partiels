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
            , DocumentExport
            
            , EditUndo
            , EditRedo
            , EditNewGroup
            , EditNewTrack
            , EditRemoveItem
            , EditLoadTemplate
            
            , TransportTogglePlayback
            , TransportToggleLooping
            , TransportRewindPlayHead
            
            , ViewZoomIn
            , ViewZoomOut
            , ViewInfoBubble
            
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
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        std::tuple<juce::String, size_t> getNewTrackPosition() const;
        void addPluginTrack(Plugin::Key const& key, Plugin::Description const& description, juce::String groupIdentifier, size_t position);
        void addFileTrack(juce::File const& file, juce::String groupIdentifier, size_t position);

        Accessor::Listener mListener;
        PluginList::Table mPluginListTable;
    };
} // namespace Application

ANALYSE_FILE_END
