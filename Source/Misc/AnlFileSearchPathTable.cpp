#include "AnlFileSearchPathTable.h"
#include <AnlIconsData.h>

MISC_FILE_BEGIN

FileSearchPathTable::Directory::Entry::Entry(size_t i, juce::String const& fileName)
: index(i)
, revealButton(juce::ImageCache::getFromMemory(AnlIconsData::folder_png, AnlIconsData::folder_pngSize))
{
    addAndMakeVisible(thumbLabel);
    thumbLabel.setEditable(false);
    thumbLabel.setText("#" + juce::String(index + 1_z), juce::NotificationType::dontSendNotification);

    addAndMakeVisible(fileNameLabel);
    fileNameLabel.setEditable(false);
    fileNameLabel.setText(fileName, juce::NotificationType::dontSendNotification);
    fileNameLabel.setTooltip(fileName);

    addAndMakeVisible(replaceButton);
    replaceButton.setButtonText(juce::translate("Change"));
    replaceButton.setWantsKeyboardFocus(false);

    addAndMakeVisible(revealButton);
    revealButton.setWantsKeyboardFocus(false);
}

void FileSearchPathTable::Directory::Entry::resized()
{
    auto bounds = getLocalBounds();
    revealButton.setBounds(bounds.removeFromRight(getHeight()).reduced(2));
    replaceButton.setBounds(bounds.removeFromRight(80).reduced(2));
    thumbLabel.setBounds(bounds.removeFromLeft(32));
    fileNameLabel.setBounds(bounds);
}

void FileSearchPathTable::Directory::Entry::paint(juce::Graphics& g)
{
    g.setColour(findColour(Decorator::ColourIds::normalBorderColourId));
    g.fillRect(getLocalBounds().removeFromLeft(thumbLabel.getWidth()));
}

FileSearchPathTable::Directory::Directory(FileSearchPathTable& owner, size_t index, juce::File const& file)
: mOwner(owner)
, mEntry(index, file.getFullPathName())
{
    mEntry.thumbLabel.addMouseListener(this, true);
    mEntry.fileNameLabel.addMouseListener(this, true);
    mEntry.revealButton.onClick = [=]()
    {
        file.revealToUser();
    };

    mEntry.replaceButton.onClick = [=, this]()
    {
        mOwner.mFileChooser = std::make_unique<juce::FileChooser>(juce::translate(juce::translate("Replace the directory...")));
        if(mOwner.mFileChooser == nullptr)
        {
            return;
        }
        using Flags = juce::FileBrowserComponent::FileChooserFlags;
        mOwner.mFileChooser->launchAsync(Flags::openMode | Flags::canSelectDirectories, [=, this](juce::FileChooser const& fileChooser)
                                         {
                                             auto const files = fileChooser.getResults();
                                             if(files.isEmpty())
                                             {
                                                 return;
                                             }
                                             auto fileSearchPath = mOwner.mFileSearchPath;
                                             MiscWeakAssert(mEntry.index < fileSearchPath.size());
                                             if(mEntry.index >= fileSearchPath.size())
                                             {
                                                 return;
                                             }
                                             fileSearchPath[mEntry.index] = files.getFirst();
                                             mOwner.setFileSearchPath(fileSearchPath);
                                         });
    };

    addAndMakeVisible(mDecorator);
    setSize(300, 24);
}

void FileSearchPathTable::Directory::mouseMove(juce::MouseEvent const& event)
{
    if(event.originalComponent == &mEntry.thumbLabel)
    {
        mEntry.thumbLabel.setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    }
}

void FileSearchPathTable::Directory::mouseDown(juce::MouseEvent const& event)
{
    auto selection = mOwner.getSelection();
    if(event.mods.isCommandDown())
    {
        if(selection.count(mEntry.index) > 0_z)
        {
            selection.erase(mEntry.index);
        }
        else
        {
            selection.insert(mEntry.index);
        }
        mOwner.setSelection(selection, juce::NotificationType::sendNotificationSync);
    }
    else if(!selection.empty() && event.mods.isShiftDown())
    {
        while(*selection.cbegin() > mEntry.index)
        {
            selection.insert(*selection.cbegin() - 1_z);
        }
        while(*selection.crbegin() < mEntry.index)
        {
            selection.insert(*selection.crbegin() + 1_z);
        }
        mOwner.setSelection(selection, juce::NotificationType::sendNotificationSync);
    }
    else
    {
        mOwner.setSelection({mEntry.index}, juce::NotificationType::sendNotificationSync);
    }

    mouseMove(event);
    beginDragAutoRepeat(5);
}

void FileSearchPathTable::Directory::mouseDrag(juce::MouseEvent const& event)
{
    mouseMove(event);
    if(event.originalComponent != &mEntry.thumbLabel)
    {
        return;
    }
    if((event.eventTime - event.mouseDownTime).inMilliseconds() < static_cast<juce::int64>(100))
    {
        return;
    }
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    MiscWeakAssert(dragContainer != nullptr);
    if(dragContainer == nullptr)
    {
        return;
    }
    if(!dragContainer->isDragAndDropActive())
    {
        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Channel", juce::String(mEntry.index), getHeight()), this, juce::ScaledImage{}, true, &p, &event.source);
    }
}

void FileSearchPathTable::Directory::mouseUp(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void FileSearchPathTable::Directory::resized()
{
    mDecorator.setBounds(getLocalBounds());
}

void FileSearchPathTable::Directory::setSelected(bool state)
{
    mDecorator.setHighlighted(state);
}

FileSearchPathTable::FileSearchPathTable(juce::File const& defaultPath)
: mDefaultPath(defaultPath)
, mAddButton(juce::ImageCache::getFromMemory(AnlIconsData::plus_png, AnlIconsData::plus_pngSize))
{
    mComponentListener.onComponentResized = [this](juce::Component&)
    {
        resized();
    };

    mDraggableTable.onComponentDropped = [this](juce::String const& identifier, size_t index, bool copy)
    {
        auto fileSearchPath = mFileSearchPath;
        auto const previousIndex = static_cast<size_t>(identifier.getIntValue());
        MiscWeakAssert(previousIndex < fileSearchPath.size());
        if(previousIndex >= fileSearchPath.size())
        {
            return;
        }
        auto const previouslayout = fileSearchPath[previousIndex];
        if(!copy)
        {
            fileSearchPath.erase(fileSearchPath.begin() + static_cast<long>(previousIndex));
        }
        fileSearchPath.insert(fileSearchPath.begin() + static_cast<long>(index), previouslayout);
        setFileSearchPath(fileSearchPath);
    };

    mAddButton.onClick = [this]()
    {
        auto const path = mDirectories.empty() ? mDefaultPath : mFileSearchPath[0];
        mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Add a folder..."), path);
        if(mFileChooser == nullptr)
        {
            return;
        }
        using Flags = juce::FileBrowserComponent::FileChooserFlags;
        mFileChooser->launchAsync(Flags::openMode | Flags::canSelectDirectories | Flags::canSelectMultipleItems, [this](juce::FileChooser const& fileChooser)
                                  {
                                      auto const files = fileChooser.getResults();
                                      auto fileSearchPath = mFileSearchPath;
                                      for(auto const& file : files)
                                      {
                                          if(file.isDirectory())
                                          {
                                              fileSearchPath.push_back(file);
                                          }
                                      }
                                      setFileSearchPath(fileSearchPath);
                                  });
    };

    mAddLabel.setInterceptsMouseClicks(false, false);
    mAddLabel.setText(juce::translate("Add directories..."), juce::NotificationType::dontSendNotification);
    mAddLabel.setTooltip(juce::translate("Add directories..."));
    mAddButton.setTooltip(juce::translate("Add directories..."));

    mComponentListener.attachTo(mDraggableTable);
    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mAddButton);
    addAndMakeVisible(mAddLabel);
    setWantsKeyboardFocus(true);
    setSize(300, 400);
}

FileSearchPathTable::~FileSearchPathTable()
{
    mComponentListener.detachFrom(mDraggableTable);
}

void FileSearchPathTable::resized()
{
    auto bounds = getLocalBounds();
    auto addBounds = bounds.removeFromBottom(24);
    mAddButton.setBounds(addBounds.removeFromLeft(24).reduced(4));
    mAddLabel.setBounds(addBounds);
    auto const scrollbarWidth = mViewport.getVerticalScrollBar().isVisible() ? mViewport.getScrollBarThickness() : 0;
    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
}

void FileSearchPathTable::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

bool FileSearchPathTable::keyPressed(juce::KeyPress const& key)
{
    if(key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        return deleteSelection();
    }
    if(key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::ctrlModifier, 0))
    {
        return copySelection();
    }
    if(key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::deleteKey, juce::ModifierKeys::shiftModifier, 0))
    {
        return cutSelection();
    }
    if(key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress(juce::KeyPress::insertKey, juce::ModifierKeys::shiftModifier, 0))
    {
        return pasteSelection();
    }
    if(key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) || key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        return mUndoManager.redo();
    }
    if(key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        return mUndoManager.undo();
    }
    if(key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
    {
        return selectionAll();
    }
    if(key.isKeyCode(juce::KeyPress::upKey))
    {
        return moveSelectionUp(key.getModifiers().isShiftDown());
    }
    if(key.isKeyCode(juce::KeyPress::downKey))
    {
        return moveSelectionDown(key.getModifiers().isShiftDown());
    }
    return false;
}

bool FileSearchPathTable::isInterestedInFileDrag(juce::StringArray const& paths)
{
    return std::any_of(paths.begin(), paths.end(), [](auto const& path)
                       {
                           return juce::File(path).isDirectory();
                       });
}

void FileSearchPathTable::fileDragEnter(juce::StringArray const& paths, int x, int y)
{
    juce::ignoreUnused(x, y);
    mIsDragging = std::any_of(paths.begin(), paths.end(), [](auto const& path)
                              {
                                  return juce::File(path).isDirectory();
                              });
    repaint();
}

void FileSearchPathTable::fileDragExit(juce::StringArray const& paths)
{
    juce::ignoreUnused(paths);
    mIsDragging = false;
    repaint();
}

void FileSearchPathTable::filesDropped(juce::StringArray const& paths, int x, int y)
{
    juce::ignoreUnused(x, y);
    auto fileSearchPath = mFileSearchPath;
    for(auto const& path : paths)
    {
        juce::File const file(path);
        if(file.isDirectory())
        {
            fileSearchPath.push_back(file);
        }
    }
    setFileSearchPath(fileSearchPath);
    fileDragExit(paths);
}

void FileSearchPathTable::setSelection(std::set<size_t> indices, juce::NotificationType notification)
{
    auto it = indices.begin();
    while(it != indices.end())
    {
        if(*it > mDirectories.size())
        {
            it = indices.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for(auto index = 0_z; index < mDirectories.size(); ++index)
    {
        if(mDirectories[index] != nullptr)
        {
            mDirectories[index]->setSelected(indices.count(index) > 0_z);
        }
    }

    if(mSelection != indices)
    {
        mSelection = indices;
        if(notification == juce::NotificationType::dontSendNotification)
        {
            return;
        }
        else if(notification == juce::NotificationType::sendNotificationAsync)
        {
            postCommandMessage(1);
        }
        else
        {
            handleCommandMessage(1);
        }
    }
}

std::set<size_t> FileSearchPathTable::getSelection() const
{
    return mSelection;
}

std::vector<juce::File> FileSearchPathTable::getFileSearchPath() const noexcept
{
    return mFileSearchPath;
}

void FileSearchPathTable::setFileSearchPath(std::vector<juce::File> const& fileSearchPath)
{
    if(mFileSearchPath == fileSearchPath)
    {
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(FileSearchPathTable& owner, std::vector<juce::File> const& l)
        : mOwner(owner)
        , mPreviousFileSearchPath(mOwner.mFileSearchPath)
        , mNewFileSearchPath(l)
        , mPreviousSelection(mOwner.mSelection)
        {
        }

        ~Action() override = default;

        // juce::UndoableAction
        bool perform() override
        {
            setFileSearchPath(mNewFileSearchPath);
            return true;
        }

        bool undo() override
        {
            setFileSearchPath(mPreviousFileSearchPath);
            mOwner.setSelection(mPreviousSelection, juce::NotificationType::sendNotificationSync);
            return true;
        }

        void setFileSearchPath(std::vector<juce::File> const& l)
        {
            mOwner.mFileSearchPath = l;
            mOwner.updateFileSearchPath();
            mOwner.handleCommandMessage(0);
        }

    private:
        FileSearchPathTable& mOwner;
        std::vector<juce::File> mPreviousFileSearchPath;
        std::vector<juce::File> mNewFileSearchPath;
        std::set<size_t> mPreviousSelection;
    };

    mUndoManager.beginNewTransaction();
    mUndoManager.perform(std::make_unique<Action>(*this, fileSearchPath).release());
}

void FileSearchPathTable::setFileSearchPath(std::vector<juce::File> const& fileSearchPath, juce::NotificationType notification)
{
    if(mFileSearchPath == fileSearchPath)
    {
        return;
    }
    mUndoManager.clearUndoHistory();
    mFileSearchPath = fileSearchPath;
    updateFileSearchPath();

    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        postCommandMessage(0);
    }
    else
    {
        handleCommandMessage(0);
    }
}

void FileSearchPathTable::handleCommandMessage(int commandId)
{
    if(commandId == 0 && onFileSearchPathChanged != nullptr)
    {
        onFileSearchPathChanged();
    }
    else if(onSelectionChanged != nullptr)
    {
        onSelectionChanged();
    }
}

void FileSearchPathTable::updateFileSearchPath()
{
    auto const selection = mSelection;
    using ComponentRef = DraggableTable::ComponentRef;
    std::vector<ComponentRef> contents;
    mDirectories.clear();
    for(auto const& file : mFileSearchPath)
    {
        auto directoryComponent = std::make_unique<Directory>(*this, static_cast<int>(contents.size()), file);
        if(directoryComponent != nullptr)
        {
            contents.push_back(*directoryComponent.get());
            mDirectories.push_back(std::move(directoryComponent));
        }
    }
    mDraggableTable.setComponents(contents);
    resized();
    setSelection(selection, juce::NotificationType::sendNotificationSync);
}

bool FileSearchPathTable::selectionAll()
{
    if(mFileSearchPath.empty())
    {
        return false;
    }
    std::set<size_t> selection;
    for(auto index = 0_z; index < mFileSearchPath.size(); ++index)
    {
        selection.insert(index);
    }
    setSelection(selection, juce::NotificationType::sendNotificationSync);
    return true;
}

bool FileSearchPathTable::moveSelectionUp(bool preserve)
{
    if(mFileSearchPath.empty())
    {
        return false;
    }
    if(mSelection.empty())
    {
        setSelection({mFileSearchPath.size() - 1_z}, juce::NotificationType::sendNotificationSync);
        return true;
    }
    if(preserve)
    {
        auto selection = mSelection;
        selection.insert(*selection.cbegin() - 1_z);
        setSelection(selection, juce::NotificationType::sendNotificationSync);
    }
    else
    {
        setSelection({*mSelection.cbegin() - 1_z}, juce::NotificationType::sendNotificationSync);
    }
    return true;
}

bool FileSearchPathTable::moveSelectionDown(bool preserve)
{
    if(mFileSearchPath.empty())
    {
        return false;
    }
    if(mSelection.empty())
    {
        setSelection({0_z}, juce::NotificationType::sendNotificationSync);
        return true;
    }
    if(preserve)
    {
        auto selection = mSelection;
        selection.insert(*selection.crbegin() + 1_z);
        setSelection(selection, juce::NotificationType::sendNotificationSync);
    }
    else
    {
        setSelection({*mSelection.crbegin() + 1_z}, juce::NotificationType::sendNotificationSync);
    }
    return true;
}

bool FileSearchPathTable::deleteSelection()
{
    if(mSelection.empty())
    {
        return false;
    }
    auto fileSearchPath = mFileSearchPath;
    for(auto it = mSelection.crbegin(); it != mSelection.crend(); ++it)
    {
        MiscWeakAssert(*it < fileSearchPath.size());
        if(*it < fileSearchPath.size())
        {
            fileSearchPath.erase(fileSearchPath.begin() + static_cast<long>(*it));
        }
    }
    setFileSearchPath(fileSearchPath);
    auto const index = *mSelection.cbegin() > 0_z && !fileSearchPath.empty() ? std::min(*mSelection.cbegin() - 1_z, fileSearchPath.size() - 1_z) : 0_z;
    setSelection(fileSearchPath.empty() ? std::set<size_t>{} : std::set<size_t>{index}, juce::NotificationType::sendNotificationSync);
    return true;
}

bool FileSearchPathTable::copySelection()
{
    if(mSelection.empty())
    {
        return false;
    }
    mClipBoard.clear();
    for(auto it = mSelection.crbegin(); it != mSelection.crend(); ++it)
    {
        MiscWeakAssert(*it < mFileSearchPath.size());
        if(*it < mFileSearchPath.size())
        {
            mClipBoard.push_back(mFileSearchPath[*it]);
        }
    }
    return true;
}

bool FileSearchPathTable::cutSelection()
{
    return copySelection() ? deleteSelection() : false;
}

bool FileSearchPathTable::pasteSelection()
{
    if(mClipBoard.empty())
    {
        return false;
    }
    auto fileSearchPath = mFileSearchPath;
    auto selection = mSelection;
    auto position = selection.empty() || *selection.cbegin() > fileSearchPath.size() ? fileSearchPath.size() : *selection.cbegin() + 1_z;
    selection.clear();
    for(auto const& channel : mClipBoard)
    {
        fileSearchPath.insert(fileSearchPath.begin() + static_cast<long>(position), channel);
        selection.insert(position);
        ++position;
    }
    setFileSearchPath(fileSearchPath);
    setSelection(selection, juce::NotificationType::sendNotificationSync);
    return true;
}

MISC_FILE_END
