#pragma once

#include "../Document/AnlDocumentFileInfo.h"
#include "../Document/AnlDocumentReaderLayout.h"
#include "../Document/AnlDocumentSection.h"
#include "../Plugin/AnlPluginListSearchPath.h"
#include "AnlApplicationAbout.h"
#include "AnlApplicationAudioSettings.h"
#include "AnlApplicationBatcher.h"
#include "AnlApplicationCoAnalyzer.h"
#include "AnlApplicationConverter.h"
#include "AnlApplicationExporter.h"
#include "AnlApplicationGraphicPreset.h"
#include "AnlApplicationKeyMappings.h"
#include "AnlApplicationLoader.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    {
    public:
        Interface();
        ~Interface() override;

        juce::Component const* getPlot(juce::String const& identifier) const;
        PluginList::Table& getPluginListTable();

        // juce::Component
        void resized() override;

        void hideCurrentPanel();
        void showAboutPanel();
        void showAudioSettingsPanel();
        void showOscSettingsPanel();
        void showCoAnalyzerSettingsPanel();
        void showGraphicPresetPanel();
        void showBatcherPanel();
        void showConverterPanel();
        void showExporterPanel();
        void showPluginSearchPathPanel();
        void showReaderLayoutPanel();
        void showDocumentFileInfoPanel();
        void showKeyMappingsPanel();
        void showTrackLoaderPanel();

        void showPluginListTablePanel();
        void hidePluginListTablePanel();
        void togglePluginListTablePanel();
        bool isPluginListTablePanelVisible() const;

        void showCoAnalyzerPanel();
        void hideCoAnalyzerPanel();
        void toggleCoAnalyzerPanel();
        bool isCoAnalyzerPanelVisible() const;

    private:
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
            ~PluginListTablePanel() override = default;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;

        private:
            juce::Label mTitleLabel;
            Icon mCloseButton;
            Icon mSettingsButton;
            ColouredPanel mTopSeparator;
            ColouredPanel mLeftSeparator;
            juce::Component& mContent;
        };

        class CoAnalyzerPanel
        : public juce::Component
        {
        public:
            CoAnalyzerPanel(juce::Component& content);
            ~CoAnalyzerPanel() override = default;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;

        private:
            juce::Label mTitleLabel;
            Icon mCloseButton;
            Icon mSettingsButton;
            ColouredPanel mTopSeparator;
            ColouredPanel mLeftSeparator;
            juce::Component& mContent;
        };

        class RightBorder
        : public juce::Component
        {
        public:
            RightBorder();
            ~RightBorder() override = default;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;

        private:
            Icon mPluginListButton;
            Icon mCoAnalyzerButton;
        };

        class DocumentContainer
        : public DragAndDropTarget
        {
        public:
            DocumentContainer();
            ~DocumentContainer() override;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

            Document::Section const& getDocumentSection() const;
            PluginList::Table& getPluginListTable();

            void setRightPanelsVisible(bool const pluginListTableVisible, bool const coAnalyzerPanelVisible);
            void showPluginListTablePanel();
            void hidePluginListTablePanel();
            void togglePluginListTablePanel();
            bool isPluginListTablePanelVisible() const;

            void showCoAnalyzerPanel();
            void hideCoAnalyzerPanel();
            void toggleCoAnalyzerPanel();
            bool isCoAnalyzerPanelVisible() const;

        private:
            Document::Accessor::Listener mDocumentListener{typeid(*this).name()};

            ColouredPanel mToolTipSeparator;
            Tooltip::Display mToolTipDisplay;
            Document::Section mDocumentSection;
            LoaderContent mLoaderContent;
            Decorator mLoaderDecorator{mLoaderContent};
            RightBorder mRightBorder;
            ColouredPanel mRightSeparator;
            PluginList::Table mPluginListTable;
            PluginListTablePanel mPluginListTablePanel{mPluginListTable};
            bool mPluginListTableVisible{false};
            CoAnalyzer::Chat mCoAnalyzerChat;
            CoAnalyzerPanel mCoAnalyzerChatPanel{mCoAnalyzerChat};
            bool mCoAnalyzerChatVisible{false};
            static auto constexpr rightPanelsWidth = 240;
        };

        Document::Accessor::Receiver mDocumentReceiver;

        DocumentContainer mDocumentContainer;
        AboutPanel mAboutPanel;
        AudioSettingsPanel mAudioSettingsPanel;
        Osc::SettingsPanel mOscSettingsPanel;
        CoAnalyzer::SettingsPanel mCoAnalyzerSettingsPanel;
        GraphicPresetPanel mGraphicPresetPanel;
        BatcherPanel mBatcherPanel;
        ConverterPanel mConverterPanel;
        ExporterPanel mExporterPanel;
        PluginSearchPathPanel mPluginSearchPathPanel;
        ReaderLayoutPanel mReaderLayoutPanel;
        Document::FileInfoPanel mDocumentFileInfoPanel;
        KeyMappingsPanel mKeyMappingsPanel;
        HideablePanelManager mPanelManager;
    };
} // namespace Application

ANALYSE_FILE_END
