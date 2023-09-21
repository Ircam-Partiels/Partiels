#pragma once

#include "../Document/AnlDocumentDirector.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class BatcherContent
    : public juce::Component
    , public juce::DragAndDropContainer
    , private juce::AsyncUpdater
    {
    public:
        BatcherContent();
        ~BatcherContent() override;

        // juce::Component
        void resized() override;

        bool canCloseWindow() const;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        void process();

        AlertWindow::Catcher mAlertCatcher;
        juce::UndoManager mUndoManager;
        Document::Accessor mDocumentAccessor;
        Document::Director mDocumentDirector;
        Accessor::Listener mListener{typeid(*this).name()};

        AudioFileLayoutTable mAudioFileLayoutTable;
        ColouredPanel mSeparator;
        Document::Exporter::Panel mExporterPanel;
        PropertyToggle mPropertyAdaptationToSampleRate;
        PropertyTextButton mPropertyExport;
        LoadingIcon mLoadingIcon;
        ComponentListener mComponentListener;
        std::unique_ptr<juce::FileChooser> mFileChooser;

        using ProcessResult = std::tuple<AlertWindow::MessageType, juce::String, juce::String>;
        std::atomic<bool> mShoulAbort{false};
        std::future<ProcessResult> mProcess;
    };

    class BatcherPanel
    : public HideablePanelTyped<BatcherContent>
    {
    public:
        BatcherPanel();
        ~BatcherPanel() override = default;

        // HideablePanel
        bool escapeKeyPressed() override;
    };
} // namespace Application

ANALYSE_FILE_END
