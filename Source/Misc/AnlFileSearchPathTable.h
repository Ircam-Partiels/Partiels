#pragma once

#include "AnlDecorator.h"
#include "AnlDraggableTable.h"

ANALYSE_FILE_BEGIN

class FileSearchPathTable
: public juce::Component
, public juce::FileDragAndDropTarget
{
public:
    FileSearchPathTable(juce::File const& defaultPath = {});
    ~FileSearchPathTable() override;

    std::vector<juce::File> getFileSearchPath() const noexcept;
    void setFileSearchPath(std::vector<juce::File> const& fileSearchPath, juce::NotificationType notification);

    void setSelection(std::set<size_t> indices, juce::NotificationType notification);
    std::set<size_t> getSelection() const;

    std::function<void(void)> onFileSearchPathChanged = nullptr;
    std::function<void(void)> onSelectionChanged = nullptr;

    // juce::Component
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;
    bool keyPressed(juce::KeyPress const& key) override;
    void handleCommandMessage(int commandId) override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(juce::StringArray const& paths) override;
    void fileDragEnter(juce::StringArray const& paths, int x, int y) override;
    void fileDragExit(juce::StringArray const& paths) override;
    void filesDropped(juce::StringArray const& paths, int x, int y) override;

private:
    void setFileSearchPath(std::vector<juce::File> const& fileSearchPath);
    void updateFileSearchPath();

    bool selectionAll();
    bool moveSelectionUp(bool preserve);
    bool moveSelectionDown(bool preserve);
    bool deleteSelection();
    bool copySelection();
    bool cutSelection();
    bool pasteSelection();

    class Directory
    : public juce::Component
    {
    public:
        Directory(FileSearchPathTable& owner, size_t index, juce::File const& file);
        ~Directory() override = default;

        void setSelected(bool state);

        // juce::Component
        void resized() override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;

    private:
        struct Entry
        : public juce::Component
        {
            Entry(size_t i, juce::String const& fileName);
            ~Entry() override = default;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

            size_t const index;
            juce::Label thumbLabel;
            juce::Label fileNameLabel;
        };

        FileSearchPathTable& mOwner;
        Entry mEntry;
        Decorator mDecorator{mEntry};
    };

    juce::File const mDefaultPath;
    std::vector<std::unique_ptr<Directory>> mDirectories;
    DraggableTable mDraggableTable{"Channel"};
    ComponentListener mComponentListener;
    juce::Viewport mViewport;
    juce::ImageButton mAddButton;
    juce::Label mAddLabel;
    std::vector<juce::File> mFileSearchPath;
    std::vector<juce::File> mClipBoard;
    std::set<size_t> mSelection;
    bool mIsDragging{false};
    std::unique_ptr<juce::FileChooser> mFileChooser;
    juce::UndoManager mUndoManager;
};

ANALYSE_FILE_END
