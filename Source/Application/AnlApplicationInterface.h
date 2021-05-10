#pragma once

#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentSection.h"
#include "../Transport/AnlTransportDisplay.h"
#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public CommandTarget
    , private juce::ComponentListener
    {
    public:
        Interface();
        ~Interface() override;

        void moveKeyboardFocusTo(juce::String const& identifier);

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;

    private:
        // juce::ComponentListener
        void componentVisibilityChanged(juce::Component& component) override;

        class Loader
        : public juce::Component
        , public juce::FileDragAndDropTarget
        , private juce::ApplicationCommandManagerListener
        {
        public:
            Loader();
            ~Loader() override;

            void updateState();

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

            // juce::FileDragAndDropTarget
            bool isInterestedInFileDrag(juce::StringArray const& files) override;
            void fileDragEnter(juce::StringArray const& files, int x, int y) override;
            void fileDragExit(juce::StringArray const& files) override;
            void filesDropped(juce::StringArray const& files, int x, int y) override;

        private:
            // juce::ApplicationCommandManagerListener
            void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
            void applicationCommandListChanged() override;

            class FileTable
            : public juce::Component
            , private juce::ListBoxModel
            {
            public:
                FileTable();
                ~FileTable() override;

                // juce::Component
                void resized() override;

                std::function<void(juce::File const& file)> onFileSelected = nullptr;

            private:
                // juce::ListBoxModel
                int getNumRows() override;
                void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
                void returnKeyPressed(int lastRowSelected) override;
                void listBoxItemDoubleClicked(int row, juce::MouseEvent const& event) override;

                Accessor::Listener mListener;
                juce::ListBox mListBox{"", this};
                JUCE_LEAK_DETECTOR(FileTable)
            };

            Document::Accessor::Listener mDocumentListener;
            
            juce::TextButton mLoadFileButton{juce::translate("Load")};
            juce::Label mLoadFileInfo{"", ""};
            juce::Label mLoadFileWildcard{""};
            juce::TextButton mAddTrackButton{juce::translate("Add Track")};
            juce::Label mAddTrackInfo{""};
            juce::TextButton mLoadTemplateButton{juce::translate("Load Template")};
            juce::Label mLoadTemplateInfo{""};
            
            juce::Label mSelectRecentDocument{"", juce::translate("Select Recent Document")};
            ColouredPanel mSeparatorVertical;
            ColouredPanel mSeparatorHorizontal;
            FileTable mFileTable;
            bool mIsDragging{false};
        };

        Accessor::Listener mListener;
        Document::Section mDocumentSection;
        Loader mLoader;
        Decorator mLoaderDecorator{mLoader, 1, 2.0f};
        ColouredPanel mToolTipSeparator;
        Tooltip::Display mToolTipDisplay;
        juce::ImageButton mTooltipButton{"TooltipButton"};
    };
} // namespace Application

ANALYSE_FILE_END
