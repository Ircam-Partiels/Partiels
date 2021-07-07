#include "AnlAudioFileLayout.h"
#include "AnlAlertWindow.h"
#include "AnlDecorator.h"
#include "AnlDraggableTable.h"
#include "AnlIconManager.h"

ANALYSE_FILE_BEGIN

template <>
void XmlParser::toXml<AudioFileLayout>(juce::XmlElement& xml, juce::Identifier const& attributeName, AudioFileLayout const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "file", value.file);
        toXml(*child, "channel", value.channel);
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<AudioFileLayout>(juce::XmlElement const& xml, juce::Identifier const& attributeName, AudioFileLayout const& defaultValue)
    -> AudioFileLayout
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    AudioFileLayout value;
    value.file = fromXml(*child, "file", defaultValue.file);
    value.channel = fromXml(*child, "channel", defaultValue.channel);
    return value;
}

AudioFileLayoutTable::Channel::Entry::Entry(size_t i, juce::String const& fileName)
: index(i)
{
    addAndMakeVisible(thumbLabel);
    thumbLabel.setEditable(false);
    thumbLabel.setText("#" + juce::String(index + 1_z), juce::NotificationType::dontSendNotification);

    addAndMakeVisible(fileNameLabel);
    fileNameLabel.setEditable(false);
    fileNameLabel.setText(fileName, juce::NotificationType::dontSendNotification);

    addAndMakeVisible(channelMenu);
    channelMenu.setJustificationType(juce::Justification::right);
    channelMenu.setWantsKeyboardFocus(false);
}

void AudioFileLayoutTable::Channel::Entry::resized()
{
    auto bounds = getLocalBounds();
    thumbLabel.setBounds(bounds.removeFromLeft(32));
    channelMenu.setBounds(bounds.removeFromRight(54));
    fileNameLabel.setBounds(bounds);
}

void AudioFileLayoutTable::Channel::Entry::paint(juce::Graphics& g)
{
    g.setColour(findColour(Decorator::ColourIds::normalBorderColourId));
    g.fillRect(getLocalBounds().removeFromLeft(thumbLabel.getWidth()));
}

AudioFileLayoutTable::Channel::Channel(AudioFileLayoutTable& owner, size_t index, AudioFileLayout audioFileLayout, SupportMode mode)
: mOwner(owner)
, mAudioFileLayout(audioFileLayout)
, mEntry(index, mAudioFileLayout.file.getFileName())
{
    auto reader = std::unique_ptr<juce::AudioFormatReader>(mOwner.mAudioFormatManager.createReaderFor(mAudioFileLayout.file));
    auto& channelMenu = mEntry.channelMenu;
    channelMenu.setEnabled(reader != nullptr);
    channelMenu.setTextWhenNoChoicesAvailable(juce::String(mAudioFileLayout.channel));
    if(reader != nullptr)
    {
        sampleRate = reader->sampleRate;
        if(reader->numChannels > 1u)
        {
            if(mode & SupportMode::supportLayoutAll)
            {
                channelMenu.addItem(juce::translate("All"), -2);
            }
            if(mode & SupportMode::supportLayoutMono)
            {
                channelMenu.addItem(juce::translate("Mono"), -1);
            }
        }
        for(auto numChannel = 1u; numChannel <= reader->numChannels; ++numChannel)
        {
            channelMenu.addItem(juce::String(numChannel), static_cast<int>(numChannel));
        }
        channelMenu.setSelectedId(mAudioFileLayout.channel < 0 ? mAudioFileLayout.channel : mAudioFileLayout.channel + 1, juce::NotificationType::dontSendNotification);
    }

    channelMenu.onChange = [this]()
    {
        auto readerLayout = mOwner.getLayout();
        anlWeakAssert(mEntry.index < readerLayout.size());
        if(mEntry.index >= readerLayout.size())
        {
            return;
        }
        auto const selectedId = mEntry.channelMenu.getSelectedId();
        auto const newChannel = selectedId < 0 ? selectedId : selectedId - 1;
        readerLayout[mEntry.index].channel = newChannel;
        mAudioFileLayout.channel = newChannel;
        mOwner.setLayout(readerLayout);
    };

    mEntry.thumbLabel.addMouseListener(this, true);
    mEntry.fileNameLabel.setEnabled(reader != nullptr);
    mEntry.fileNameLabel.addMouseListener(this, true);

    addAndMakeVisible(mDecorator);
    setSize(300, 24);
}

void AudioFileLayoutTable::Channel::mouseMove(juce::MouseEvent const& event)
{
    if(event.originalComponent == &mEntry.thumbLabel)
    {
        mEntry.thumbLabel.setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
    }
}

void AudioFileLayoutTable::Channel::mouseDown(juce::MouseEvent const& event)
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

    if(event.originalComponent == &mEntry.fileNameLabel)
    {
        if(!mAudioFileLayout.file.existsAsFile())
        {
            if(AlertWindow::showOkCancel(AlertWindow::MessageType::warning, "Audio file cannot be found!", "The audio file FILENAME has been moved or deleted. Would you like to restore  it?", {{"FILENAME", mAudioFileLayout.file.getFullPathName()}}))
            {
                auto const audioFormatWildcard = mOwner.mAudioFormatManager.getWildcardForAllFormats();
                juce::FileChooser fc(juce::translate("Restore the audio file..."), mAudioFileLayout.file, audioFormatWildcard);
                if(fc.browseForFileToOpen())
                {
                    auto const newFile = fc.getResult();
                    auto layout = mOwner.mLayout;
                    for(auto& copyChannelLayout : layout)
                    {
                        if(copyChannelLayout.file == mAudioFileLayout.file)
                        {
                            copyChannelLayout.file = newFile;
                        }
                    }
                    mOwner.setLayout(layout);
                }
            }
        }
        return;
    }
    mouseMove(event);
    beginDragAutoRepeat(5);
}

void AudioFileLayoutTable::Channel::mouseDrag(juce::MouseEvent const& event)
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
    anlWeakAssert(dragContainer != nullptr);
    if(dragContainer == nullptr)
    {
        return;
    }
    if(!dragContainer->isDragAndDropActive())
    {
        juce::Image snapshot(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        paintEntireComponent(g, false);
        g.endTransparencyLayer();

        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Channel", juce::String(mEntry.index), getHeight()), this, snapshot, true, &p, &event.source);
    }
}

void AudioFileLayoutTable::Channel::mouseUp(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void AudioFileLayoutTable::Channel::resized()
{
    mDecorator.setBounds(getLocalBounds());
}

void AudioFileLayoutTable::Channel::setSelected(bool state)
{
    mDecorator.setHighlighted(state);
}

AudioFileLayoutTable::AudioFileLayoutTable(juce::AudioFormatManager& audioFormatManager, SupportMode mode, AudioFileLayout::ChannelLayout preferredChannelLayout)
: mAudioFormatManager(audioFormatManager)
, mSupportMode(mode)
, mPreferredChannelLayout(preferredChannelLayout)
{
    mBoundsListener.onComponentResized = [this](juce::Component&)
    {
        resized();
    };

    mDraggableTable.onComponentDropped = [this](juce::String const& identifier, size_t index, bool copy)
    {
        auto readerLayout = mLayout;
        auto const previousIndex = static_cast<size_t>(identifier.getIntValue());
        anlWeakAssert(previousIndex < readerLayout.size());
        if(previousIndex >= readerLayout.size())
        {
            return;
        }
        auto const previouslayout = readerLayout[previousIndex];
        if(!copy)
        {
            readerLayout.erase(readerLayout.begin() + static_cast<long>(previousIndex));
        }
        readerLayout.insert(readerLayout.begin() + static_cast<long>(index), previouslayout);
        setLayout(readerLayout);
    };

    mAddButton.onClick = [this]()
    {
        auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
        juce::FileChooser fc("Load audio files...", {}, wildcard);
        if(!fc.browseForMultipleFilesOrDirectories())
        {
            return;
        }
        auto readerLayout = mLayout;
        auto const newLayouts = getAudioFileLayouts(mAudioFormatManager, fc.getResults(), mPreferredChannelLayout);
        for(auto const& newLayout : newLayouts)
        {
            readerLayout.push_back(newLayout);
        }
        setLayout(readerLayout);
    };

    mAddLabel.setInterceptsMouseClicks(false, false);
    mAddLabel.setText(juce::translate("Insert audio files..."), juce::NotificationType::dontSendNotification);
    mAddLabel.setTooltip(juce::translate("Insert audio files..."));
    mAddButton.setTooltip(juce::translate("Insert audio files..."));

    mBoundsListener.attachTo(mDraggableTable);
    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mAddButton);
    addAndMakeVisible(mAddLabel);
    addChildComponent(mAlertLabel);
    addChildComponent(mAlertButton);
    setWantsKeyboardFocus(true);
    setSize(300, 400);
}

AudioFileLayoutTable::~AudioFileLayoutTable()
{
    mBoundsListener.detachFrom(mDraggableTable);
}

void AudioFileLayoutTable::resized()
{
    auto bounds = getLocalBounds();
    if(mAlertButton.isVisible())
    {
        auto alertBounds = bounds.removeFromBottom(24);
        mAlertButton.setBounds(alertBounds.removeFromLeft(24).reduced(4));
        mAlertLabel.setBounds(alertBounds);
    }
    auto addBounds = bounds.removeFromBottom(24);
    mAddButton.setBounds(addBounds.removeFromLeft(24).reduced(4));
    mAddLabel.setBounds(addBounds);
    auto const scrollbarWidth = mViewport.getVerticalScrollBar().isVisible() ? mViewport.getScrollBarThickness() : 0;
    mViewport.setBounds(bounds);
    mDraggableTable.setBounds(0, 0, bounds.getWidth() - scrollbarWidth, mDraggableTable.getHeight());
}

void AudioFileLayoutTable::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

void AudioFileLayoutTable::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mAddButton, IconManager::IconType::plus);
        laf->setButtonIcon(mAlertButton, IconManager::IconType::alert);
    }
}

bool AudioFileLayoutTable::keyPressed(juce::KeyPress const& key)
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

void AudioFileLayoutTable::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

bool AudioFileLayoutTable::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    for(auto const& result : files)
    {
        juce::File const file(result);
        if(file.isDirectory() || wildcard.contains(file.getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void AudioFileLayoutTable::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mIsDragging = true;
    repaint();
}

void AudioFileLayoutTable::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    mIsDragging = false;
    repaint();
}

void AudioFileLayoutTable::filesDropped(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    auto readerLayout = mLayout;
    auto const newLayouts = getAudioFileLayouts(mAudioFormatManager, files, mPreferredChannelLayout);
    for(auto const& newLayout : newLayouts)
    {
        readerLayout.push_back(newLayout);
    }
    setLayout(readerLayout);
    mIsDragging = false;
    repaint();
}

void AudioFileLayoutTable::setSelection(std::set<size_t> indices, juce::NotificationType notification)
{
    auto it = indices.begin();
    while(it != indices.end())
    {
        if(*it > mChannels.size())
        {
            it = indices.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for(auto index = 0_z; index < mChannels.size(); ++index)
    {
        if(mChannels[index] != nullptr)
        {
            mChannels[index]->setSelected(indices.count(index) > 0_z);
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

std::set<size_t> AudioFileLayoutTable::getSelection() const
{
    return mSelection;
}

std::vector<AudioFileLayout> AudioFileLayoutTable::getLayout() const
{
    return mLayout;
}

void AudioFileLayoutTable::setLayout(std::vector<AudioFileLayout> const& layout)
{
    if(mLayout == layout)
    {
        return;
    }

    class Action
    : public juce::UndoableAction
    {
    public:
        Action(AudioFileLayoutTable& owner, std::vector<AudioFileLayout> const& l)
        : mOwner(owner)
        , mPreviousLayout(mOwner.mLayout)
        , mNewLayout(l)
        , mPreviousSelection(mOwner.mSelection)
        {
        }

        ~Action() override = default;

        // juce::UndoableAction
        bool perform() override
        {
            setLayout(mNewLayout);
            return true;
        }

        bool undo() override
        {
            setLayout(mPreviousLayout);
            mOwner.setSelection(mPreviousSelection, juce::NotificationType::sendNotificationSync);
            return true;
        }

        void setLayout(std::vector<AudioFileLayout> const& l)
        {
            mOwner.clearFilesToWatch();
            mOwner.mLayout = l;
            for(auto const& channel : mOwner.mLayout)
            {
                mOwner.addFileToWatch(channel.file);
            }
            mOwner.updateLayout();
            mOwner.handleCommandMessage(0);
        }

    private:
        AudioFileLayoutTable& mOwner;
        std::vector<AudioFileLayout> mPreviousLayout;
        std::vector<AudioFileLayout> mNewLayout;
        std::set<size_t> mPreviousSelection;
    };

    mUndoManager.beginNewTransaction();
    mUndoManager.perform(std::make_unique<Action>(*this, layout).release());
}

void AudioFileLayoutTable::setLayout(std::vector<AudioFileLayout> const& layout, juce::NotificationType notification)
{
    if(mLayout == layout)
    {
        return;
    }
    mUndoManager.clearUndoHistory();
    clearFilesToWatch();
    mLayout = layout;
    for(auto const& channel : mLayout)
    {
        addFileToWatch(channel.file);
    }
    updateLayout();

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

void AudioFileLayoutTable::handleCommandMessage(int commandId)
{
    if(commandId == 0 && onLayoutChanged != nullptr)
    {
        onLayoutChanged();
    }
    else if(onSelectionChanged != nullptr)
    {
        onSelectionChanged();
    }
}

void AudioFileLayoutTable::updateLayout()
{
    auto const selection = mSelection;
    using ComponentRef = DraggableTable::ComponentRef;
    std::vector<ComponentRef> contents;
    mChannels.clear();
    for(auto const& channel : mLayout)
    {
        auto channelComponent = std::make_unique<Channel>(*this, static_cast<int>(contents.size()), channel, mSupportMode);
        if(channelComponent != nullptr)
        {
            contents.push_back(*channelComponent.get());
            mChannels.push_back(std::move(channelComponent));
        }
    }
    mDraggableTable.setComponents(contents);

    auto const hasInvalidReader = std::any_of(mChannels.cbegin(), mChannels.cend(), [](auto const& channel)
                                              {
                                                  return channel->sampleRate <= 0.0;
                                              });
    auto const currentSampleRate = std::accumulate(mChannels.cbegin(), mChannels.cend(), 0.0, [](double sr, auto const& channel)
                                                   {
                                                       return std::max(channel->sampleRate, sr);
                                                   });
    auto const hasInvalidSampleRate = !(mSupportMode & supportMultipleSampleRates) && std::any_of(mChannels.cbegin(), mChannels.cend(), [currentSampleRate](auto const& channel)
                                                                                                  {
                                                                                                      return std::abs(currentSampleRate - channel->sampleRate) > std::numeric_limits<double>::epsilon();
                                                                                                  });
    mAlertLabel.setVisible(hasInvalidReader || hasInvalidSampleRate);
    if(hasInvalidReader && hasInvalidSampleRate)
    {
        mAlertLabel.setText(juce::translate("One or more audio file cannot be allocated and the sample rates of the audio files are not consistent."), juce::NotificationType::dontSendNotification);
    }
    else if(hasInvalidReader)
    {
        mAlertLabel.setText(juce::translate("One or more audio file cannot be allocated."), juce::NotificationType::dontSendNotification);
    }
    else if(hasInvalidSampleRate)
    {
        mAlertLabel.setText(juce::translate("The sample rates of the audio files are not consistent."), juce::NotificationType::dontSendNotification);
    }
    mAlertButton.setTooltip(mAlertLabel.getText());
    mAlertButton.setVisible(mAlertLabel.isVisible());
    resized();

    setSelection(selection, juce::NotificationType::sendNotificationSync);
}

bool AudioFileLayoutTable::fileHasBeenRemoved(juce::File const& file)
{
    juce::ignoreUnused(file);
    updateLayout();
    return true;
}

bool AudioFileLayoutTable::fileHasBeenRestored(juce::File const& file)
{
    juce::ignoreUnused(file);
    updateLayout();
    return true;
}

bool AudioFileLayoutTable::fileHasBeenModified(juce::File const& file)
{
    juce::ignoreUnused(file);
    updateLayout();
    return true;
}

bool AudioFileLayoutTable::selectionAll()
{
    if(mLayout.empty())
    {
        return false;
    }
    std::set<size_t> selection;
    for(auto index = 0_z; index < mLayout.size(); ++index)
    {
        selection.insert(index);
    }
    setSelection(selection, juce::NotificationType::sendNotificationSync);
    return true;
}

bool AudioFileLayoutTable::moveSelectionUp(bool preserve)
{
    if(mLayout.empty())
    {
        return false;
    }
    if(mSelection.empty())
    {
        setSelection({mLayout.size() - 1_z}, juce::NotificationType::sendNotificationSync);
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

bool AudioFileLayoutTable::moveSelectionDown(bool preserve)
{
    if(mLayout.empty())
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

bool AudioFileLayoutTable::deleteSelection()
{
    if(mSelection.empty())
    {
        return false;
    }
    auto layout = getLayout();
    for(auto it = mSelection.crbegin(); it != mSelection.crend(); ++it)
    {
        anlWeakAssert(*it < layout.size());
        if(*it < layout.size())
        {
            layout.erase(layout.begin() + static_cast<long>(*it));
        }
    }
    setLayout(layout);
    auto const index = *mSelection.cbegin() > 0_z && !layout.empty() ? std::min(*mSelection.cbegin() - 1_z, layout.size() - 1_z) : 0_z;
    setSelection(layout.empty() ? std::set<size_t>{} : std::set<size_t>{index}, juce::NotificationType::sendNotificationSync);
    return true;
}

bool AudioFileLayoutTable::copySelection()
{
    if(mSelection.empty())
    {
        return false;
    }
    mClipBoard.clear();
    for(auto it = mSelection.crbegin(); it != mSelection.crend(); ++it)
    {
        anlWeakAssert(*it < mLayout.size());
        if(*it < mLayout.size())
        {
            mClipBoard.push_back(mLayout[*it]);
        }
    }
    return true;
}

bool AudioFileLayoutTable::cutSelection()
{
    return copySelection() ? deleteSelection() : false;
}

bool AudioFileLayoutTable::pasteSelection()
{
    if(mClipBoard.empty())
    {
        return false;
    }
    auto layout = getLayout();
    auto selection = mSelection;
    auto position = selection.empty() || *selection.cbegin() > layout.size() ? layout.size() : *selection.cbegin() + 1_z;
    selection.clear();
    for(auto const& channel : mClipBoard)
    {
        layout.insert(layout.begin() + static_cast<long>(position), channel);
        selection.insert(position);
        ++position;
    }
    setLayout(layout);
    setSelection(selection, juce::NotificationType::sendNotificationSync);
    return true;
}

std::vector<AudioFileLayout> getAudioFileLayouts(juce::AudioFormatManager& audioFormatManager, juce::Array<juce::File> const& files, AudioFileLayout::ChannelLayout preferredChannelLayout)
{
    std::vector<AudioFileLayout> audioFiles;
    auto addToAudioFiles = [&](juce::File const& file)
    {
        auto reader = std::unique_ptr<juce::AudioFormatReader>(audioFormatManager.createReaderFor(file));
        if(reader != nullptr)
        {
            auto const mode = reader->numChannels > 1 ? preferredChannelLayout : AudioFileLayout::ChannelLayout::split;
            switch(mode)
            {
                case AudioFileLayout::ChannelLayout::all:
                {
                    audioFiles.emplace_back(file, AudioFileLayout::ChannelLayout::all);
                }
                break;
                case AudioFileLayout::ChannelLayout::mono:
                {
                    audioFiles.emplace_back(file, AudioFileLayout::ChannelLayout::mono);
                }
                break;
                case AudioFileLayout::ChannelLayout::split:
                {
                    for(unsigned int channel = 0; channel < reader->numChannels; ++channel)
                    {
                        audioFiles.emplace_back(file, static_cast<int>(channel));
                    }
                }
                break;
            }
        }
    };
    auto const wildcardForAllFormats = audioFormatManager.getWildcardForAllFormats();
    for(auto const& file : files)
    {
        if(file.isDirectory())
        {
            auto const childs = file.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, wildcardForAllFormats);
            for(auto const& child : childs)
            {
                addToAudioFiles(child);
            }
        }
        else if(file.existsAsFile() && wildcardForAllFormats.contains(file.getFileExtension()))
        {
            addToAudioFiles(file);
        }
    }
    return audioFiles;
}

std::vector<AudioFileLayout> getAudioFileLayouts(juce::AudioFormatManager& audioFormatManager, juce::StringArray const& files, AudioFileLayout::ChannelLayout preferredChannelLayout)
{
    juce::Array<juce::File> rfiles;
    rfiles.ensureStorageAllocated(files.size());
    for(auto const& file : files)
    {
        rfiles.add({file});
    }
    return getAudioFileLayouts(audioFormatManager, rfiles, preferredChannelLayout);
}

ANALYSE_FILE_END
