#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "../Document/AnlDocumentTools.h"
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
        using CommandIDs = ApplicationCommandIDs;

        CommandTarget();
        ~CommandTarget() override;

        // juce::ApplicationCommandTarget
        juce::ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

        void selectDefaultTemplateFile();
        void selectQuickExportDirectory();

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        Accessor::Listener mListener{typeid(*this).name()};
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Document::Accessor::Listener mDocumentListener{typeid(*this).name()};
        std::unique_ptr<juce::FileChooser> mFileChooser;
        Document::Tools::Clipboard mClipboard;
        std::atomic<bool> mShouldAbort{false};

        JUCE_DECLARE_WEAK_REFERENCEABLE(CommandTarget)
    };
} // namespace Application

ANALYSE_FILE_END
