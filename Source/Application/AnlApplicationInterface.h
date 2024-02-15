#pragma once

#include "../Document/AnlDocumentReaderLayout.h"
#include "../Document/AnlDocumentSection.h"
#include "../Plugin/AnlPluginListSearchPath.h"
#include "AnlApplicationAbout.h"
#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationBatcher.h"
#include "AnlApplicationCommandTarget.h"
#include "AnlApplicationConverter.h"
#include "AnlApplicationExporter.h"
#include "AnlApplicationKeyMappings.h"
#include "AnlApplicationLoader.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public CommandTarget
    {
    public:
        Interface();
        ~Interface() override;

        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;
        PluginList::Table& getPluginListTable();
        Track::Loader::ArgumentSelector& getTrackLoaderArgumentSelector();

        // juce::Component
        void resized() override;

        void hideCurrentPanel();
        void showAboutPanel();
        void showAudioSettingsPanel();
        void showBatcherPanel();
        void showConverterPanel();
        void showExporterPanel();
        void showPluginSearchPathPanel();
        void showReaderLayoutPanel();
        void showKeyMappingsPanel();
        void showTrackLoaderPanel();

        void showPluginListTablePanel();
        void hidePluginListTablePanel();
        void togglePluginListTablePanel();
        bool isPluginListTablePanelVisible() const;

    private:
        class TrackLoaderPanel
        : public HideablePanelTyped<Track::Loader::ArgumentSelector>
        {
        public:
            TrackLoaderPanel();
            ~TrackLoaderPanel() override = default;

            Track::Loader::ArgumentSelector& getArgumentSelector();
        };

        class ReaderLayoutPanel
        : public HideablePanel
        {
        public:
            ReaderLayoutPanel();
            ~ReaderLayoutPanel() override;

            // HideablePanel
            bool escapeKeyPressed() override;

        private:
            Document::ReaderLayoutContent mReaderLayoutContent;
        };

        class PluginSearchPathPanel
        : public HideablePanel
        {
        public:
            PluginSearchPathPanel();
            ~PluginSearchPathPanel() override;

            // HideablePanel
            bool escapeKeyPressed() override;

        private:
            PluginList::SearchPath mPluginSearchPath;
        };

        class PluginListTablePanel
        : public juce::Component
        {
        public:
            PluginListTablePanel(juce::Component& content);

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;

        private:
            juce::Label mTitleLabel;
            Icon mSettingsButton;
            ColouredPanel mTopSeparator;
            ColouredPanel mLeftSeparator;
            juce::Component& mContent;
        };

        class DocumentContainer
        : public DragAndDropTarget
        {
        public:
            DocumentContainer();
            ~DocumentContainer() override;

            // juce::Component
            void resized() override;

            Document::Section const& getDocumentSection() const;
            PluginList::Table& getPluginListTable();

            void showPluginListTablePanel();
            void hidePluginListTablePanel();
            void togglePluginListTablePanel();
            bool isPluginListTablePanelVisible() const;

        private:
            Document::Accessor::Listener mDocumentListener{typeid(*this).name()};

            ColouredPanel mToolTipSeparator;
            Tooltip::Display mToolTipDisplay;
            Document::Section mDocumentSection;
            LoaderContent mLoaderContent;
            Decorator mLoaderDecorator{mLoaderContent};
            PluginList::Table mPluginListTable;
            PluginListTablePanel mPluginListTablePanel{mPluginListTable};
            bool mPluginListTableVisible{false};
            static auto constexpr pluginListTableWidth = 240;
        };

        Document::Accessor::Receiver mDocumentReceiver;

        DocumentContainer mDocumentContainer;
        AboutPanel mAboutPanel;
        AudioSettingsPanel mAudioSettingsPanel;
        BatcherPanel mBatcherPanel;
        ConverterPanel mConverterPanel;
        ExporterPanel mExporterPanel;
        PluginSearchPathPanel mPluginSearchPathPanel;
        ReaderLayoutPanel mReaderLayoutPanel;
        KeyMappingsPanel mKeyMappingsPanel;
        TrackLoaderPanel mTrackLoaderPanel;
        HideablePanelManager mPanelManager;

        Tooltip::BasicWindow mPanelTooltipWindow{this};
    };
} // namespace Application

ANALYSE_FILE_END
