#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Plugin/AnlPluginListSearchPath.h"
#include "../Plugin/AnlPluginListTable.h"
#include "AnlApplicationAbout.h"
#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationBatcher.h"
#include "AnlApplicationConverterPanel.h"
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

        PluginList::Table mPluginListTable;
        PluginList::Table::WindowContainer mPluginListTableWindow{mPluginListTable};
        juce::TooltipWindow mPluginListTableTooltipWindow{&mPluginListTable};
        using PluginTableContainer = Document::Director::PluginTableContainer;
        PluginTableContainer mPluginTableContainer{mPluginListTable, mPluginListTableWindow};

        PluginList::SearchPath mPluginListSearchPath;
        PluginList::SearchPath::WindowContainer mPluginListSearchPathWindow{mPluginListSearchPath};

        About mAbout;
        About::WindowContainer mAboutWindow{mAbout};

        Exporter mExporter;
        Exporter::WindowContainer mExporterWindow{mExporter};

        Batcher mBatcher;
        Batcher::WindowContainer mBatcherWindow{mBatcher};

        ConverterPanel mSdifConverter;
        FloatingWindowContainer mSdifConverterWindow;

        AudioSettings mAudioSettings;
        FloatingWindowContainer mAudioSettingsWindow;

        std::unique_ptr<juce::FileChooser> mFileChooser;
    };
} // namespace Application

ANALYSE_FILE_END
