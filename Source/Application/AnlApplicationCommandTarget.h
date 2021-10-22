#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Plugin/AnlPluginListSearchPath.h"
#include "../Plugin/AnlPluginListTable.h"
#include "AnlApplicationConverterPanel.h"
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
              documentNew = 0x2001
            , documentOpen
            , documentSave
            , documentDuplicate
            , documentConsolidate
            , documentExport
            , documentImport
            , documentBatch
            
            , editUndo
            , editRedo
            , editNewGroup
            , editNewTrack
            , editRemoveItem
            , editLoadTemplate
            
            , transportTogglePlayback
            , transportToggleLooping
            , transportRewindPlayHead
            
            , viewZoomIn
            , viewZoomOut
            , viewInfoBubble
            
            , helpOpenAudioSettings
            , helpOpenPluginSettings
            , helpOpenAbout
            , helpOpenProjectPage
            , helpSdifConverter
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
        PluginList::Table mPluginListTable;
        PluginList::SearchPath mPluginListSearchPath;
        ConverterPanel mSdifConverter;
        std::unique_ptr<juce::FileChooser> mFileChooser;
        std::unique_ptr<juce::Component> mSdifSelector;

        JUCE_DECLARE_WEAK_REFERENCEABLE(CommandTarget)
    };
} // namespace Application

ANALYSE_FILE_END
