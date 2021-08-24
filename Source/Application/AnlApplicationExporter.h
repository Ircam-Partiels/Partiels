#pragma once

#include "../Document/AnlDocumentExporter.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Exporter
    : public FloatingWindowContainer
    , private juce::AsyncUpdater
    {
    public:
        Exporter();
        ~Exporter() override;

        // juce::Component
        void resized() override;

        // FloatingWindowContainer
        void showAt(juce::Point<int> const& pt) override;
        void hide() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        void exportToFile();

        Accessor::Listener mListener{typeid(*this).name()};
        Document::Exporter::Panel mExporterPanel;
        PropertyTextButton mPropertyExport;
        LoadingCircle mLoadingCircle;
        ComponentListener mComponentListener;

        using ProcessResult = std::tuple<AlertWindow::MessageType, juce::String, juce::String>;

        std::atomic<bool> mShoulAbort{false};
        std::future<ProcessResult> mProcess;
        std::unique_ptr<juce::FileChooser> mFileChooser;
    };
} // namespace Application

ANALYSE_FILE_END
