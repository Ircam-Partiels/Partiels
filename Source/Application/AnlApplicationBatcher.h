#pragma once

#include "../Document/AnlDocumentDirector.h"
#include "../Document/AnlDocumentExporter.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Batcher
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    , private juce::AsyncUpdater
    {
    public:
        Batcher();
        ~Batcher() override;

        // juce::Component
        void resized() override;

        // FloatingWindowContainer
        void showAt(juce::Point<int> const& pt) override;
        void hide() override;

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
        LoadingCircle mLoadingCircle;
        BoundsListener mBoundsListener;

        using ProcessResult = std::tuple<AlertWindow::MessageType, juce::String, juce::String>;
        std::atomic<bool> mShoulAbort{false};
        std::future<ProcessResult> mProcess;
    };
} // namespace Application

ANALYSE_FILE_END
