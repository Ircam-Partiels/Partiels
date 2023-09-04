#pragma once

#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LoaderContent
    : public juce::Component
    , public juce::FileDragAndDropTarget
    , private juce::ApplicationCommandManagerListener
    {
    public:
        LoaderContent();
        ~LoaderContent() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;

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

            Accessor::Listener mListener{typeid(*this).name()};
            juce::ListBox mListBox{"", this};
            JUCE_LEAK_DETECTOR(FileTable)
        };

        Accessor::Listener mListener{typeid(*this).name()};
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
        bool mIsDragging{false};
    };
} // namespace Application

ANALYSE_FILE_END
