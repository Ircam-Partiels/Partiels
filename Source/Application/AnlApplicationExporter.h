#pragma once

#include "../Document/AnlDocumentExporter.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class ExporterContent
    : public juce::Component
    , private juce::AsyncUpdater
    {
    public:
        ExporterContent();
        ~ExporterContent() override;

        // juce::Component
        void resized() override;

        bool canCloseWindow() const;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        void exportToFile();

        Accessor::Listener mListener{typeid(*this).name()};
        Document::Exporter::Panel mExporterPanel;
        PropertyTextButton mPropertyExport;
        LoadingIcon mLoadingIcon;
        ComponentListener mComponentListener;

        using ProcessResult = std::tuple<AlertWindow::MessageType, juce::String, juce::String>;

        std::atomic<bool> mShoulAbort{false};
        std::future<ProcessResult> mProcess;
        std::unique_ptr<juce::FileChooser> mFileChooser;
    };

    class ExporterPanel
    : public HideablePanelTyped<ExporterContent>
    {
    public:
        ExporterPanel();
        ~ExporterPanel() override = default;

        // HideablePanel
        bool escapeKeyPressed() override;
    };
} // namespace Application

ANALYSE_FILE_END
