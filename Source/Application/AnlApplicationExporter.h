#pragma once

#include "../Document/AnlDocumentExporter.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Exporter
    : public juce::Component
    , private juce::AsyncUpdater
    {
    public:
        Exporter();
        ~Exporter() override;

        // juce::Component
        void resized() override;

        bool canCloseWindow() const;

        class WindowContainer
        : public FloatingWindowContainer
        {
        public:
            WindowContainer(Exporter& exporer);
            ~WindowContainer() override;

        private:
            Exporter& mExporter;
            juce::TooltipWindow mTooltip;
        };

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
