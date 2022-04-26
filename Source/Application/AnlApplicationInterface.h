#pragma once

#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentSection.h"
#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public juce::FileDragAndDropTarget
    , public CommandTarget
    {
    public:
        Interface();
        ~Interface() override;

        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;

        // juce::Component
        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;

        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragMove(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;

    private:
        class Loader
        : public juce::Component
        , private juce::ApplicationCommandManagerListener
        {
        public:
            Loader();
            ~Loader() override;

            void updateState();

            // juce::Component
            void resized() override;

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

                Accessor::Listener mListener{typeid(*this).name()};
                juce::ListBox mListBox{"", this};
                JUCE_LEAK_DETECTOR(FileTable)
            };

            Document::Accessor::Listener mDocumentListener{typeid(*this).name()};

            juce::TextButton mLoadFileButton{juce::translate("Load")};
            juce::Label mLoadFileInfo{"", ""};
            juce::Label mLoadFileWildcard{""};
            juce::TextButton mAddTrackButton{juce::translate("Add Track")};
            juce::Label mAddTrackInfo{""};
            juce::TextButton mLoadTemplateButton{juce::translate("Load Template")};
            juce::Label mLoadTemplateInfo{""};

            juce::Label mSelectRecentDocument;
            ColouredPanel mSeparatorVertical;
            ColouredPanel mSeparatorTop;
            ColouredPanel mSeparatorBottom;
            FileTable mFileTable;
            juce::ToggleButton mAdaptationButton;
            juce::Label mAdaptationInfo;

            Accessor::Listener mListener{typeid(*this).name()};
        };

        Accessor::Listener mListener{typeid(*this).name()};
        Document::Section mDocumentSection;
        ComponentListener mComponentListener;
        Loader mLoader;
        Decorator mLoaderDecorator{mLoader};
        ColouredPanel mToolTipSeparator;
        Tooltip::Display mToolTipDisplay;
        bool mIsDragging{false};
    };
} // namespace Application

ANALYSE_FILE_END
