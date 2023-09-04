#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Plugin/AnlPluginListTable.h"
#include "AnlApplicationExporter.h"
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
              documentNew = 0x3001
            , documentOpen
            , documentSave
            , documentDuplicate
            , documentConsolidate
            , documentExport
            , documentImport
            , documentBatch
            
            , editUndo
            , editRedo
            , editSelectNextItem
            , editNewGroup
            , editNewTrack
            , editRemoveItem
            , editLoadTemplate
            
            , transportTogglePlayback
            , transportToggleLooping
            , transportToggleStopAtLoopEnd
            , transportToggleMagnetism
            , transportRewindPlayHead
            , transportMovePlayHeadBackward
            , transportMovePlayHeadForward
            
            , viewTimeZoomIn
            , viewTimeZoomOut
            , viewVerticalZoomIn
            , viewVerticalZoomOut
            , viewInfoBubble
            
            , helpOpenAudioSettings
            , helpOpenPluginSettings
            , helpOpenAbout
            , helpOpenProjectPage
            , helpAuthorize
            , helpSdifConverter
            , helpAutoUpdate
            , helpCheckForUpdate
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

        Accessor::Listener mListener{typeid(*this).name()};
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};

        std::unique_ptr<juce::FileChooser> mFileChooser;

        JUCE_DECLARE_WEAK_REFERENCEABLE(CommandTarget)
    };
} // namespace Application

ANALYSE_FILE_END
