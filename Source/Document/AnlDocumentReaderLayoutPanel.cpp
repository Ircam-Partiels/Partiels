#include "AnlDocumentReaderLayoutPanel.h"

ANALYSE_FILE_BEGIN

Document::ReaderLayoutPanel::FileInfoPanel::FileInfoPanel()
{
    mViewport.setViewedComponent(&mConcertinaTable, false);
    addAndMakeVisible(mViewport);
    setSize(300, 400);
    setWantsKeyboardFocus(false);
}

void Document::ReaderLayoutPanel::FileInfoPanel::setAudioFormatReader(juce::File const& file, juce::AudioFormatReader const* reader)
{
    mConcertinaTable.setComponents({});
    mPanelFilePath.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);
    mPanelFilePath.entry.setEditable(false);
    mPanelFilePath.entry.setTooltip(file.getFullPathName());

    if(reader == nullptr)
    {
        resized();
        return;
    }
    mPanelFileFormat.entry.setText(reader->getFormatName(), juce::NotificationType::dontSendNotification);
    mPanelFileFormat.entry.setEditable(false);
    mPanelSampleRate.entry.setText(juce::String(reader->sampleRate, 1) + "Hz", juce::NotificationType::dontSendNotification);
    mPanelSampleRate.entry.setEditable(false);
    mPanelBitPerSample.entry.setText(juce::String(reader->bitsPerSample) + " (" + (reader->usesFloatingPointData ? "float" : "int") + ")", juce::NotificationType::dontSendNotification);
    mPanelBitPerSample.entry.setEditable(false);
    mPanelLengthInSamples.entry.setText(juce::String(reader->lengthInSamples) + " samples", juce::NotificationType::dontSendNotification);
    mPanelLengthInSamples.entry.setEditable(false);
    mPanelDurationInSeconds.entry.setText(juce::String(static_cast<double>(reader->lengthInSamples) / reader->sampleRate, 3).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "s", juce::NotificationType::dontSendNotification);
    mPanelDurationInSeconds.entry.setEditable(false);
    mPanelNumChannels.entry.setText(juce::String(reader->numChannels), juce::NotificationType::dontSendNotification);
    mPanelNumChannels.entry.setEditable(false);

    auto const& metadataValues = reader->metadataValues;
    mMetaDataPanels.clear();
    std::vector<ConcertinaTable::ComponentRef> panels{mPanelFilePath, mPanelFileFormat, mPanelSampleRate, mPanelBitPerSample, mPanelLengthInSamples, mPanelDurationInSeconds, mPanelNumChannels};

    for(auto const& key : metadataValues.getAllKeys())
    {
        auto const& value = metadataValues[key];
        auto property = std::make_unique<PropertyLabel>(key, juce::translate("Metadata MDNM of the audio file").replace("MDNM", key));
        anlStrongAssert(property != nullptr);
        if(property != nullptr)
        {
            property->entry.setText(value, juce::NotificationType::dontSendNotification);
            property->entry.setJustificationType(juce::Justification::right);
            property->entry.setEditable(false);
            panels.push_back(*property.get());
            mMetaDataPanels.push_back(std::move(property));
        }
    }
    mConcertinaTable.setComponents(panels);
    resized();
}

void Document::ReaderLayoutPanel::FileInfoPanel::resized()
{
    auto const scrollbarThickness = mViewport.getScrollBarThickness();
    mConcertinaTable.setBounds(getLocalBounds().withHeight(mConcertinaTable.getHeight()).withTrimmedRight(scrollbarThickness));
    mViewport.setBounds(getLocalBounds());
}

Document::ReaderLayoutPanel::Channel::Entry::Entry()
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    addAndMakeVisible(thumbLabel);
    addAndMakeVisible(fileNameLabel);
    addAndMakeVisible(channelMenu);
    
    thumbLabel.setEditable(false);
    fileNameLabel.setEditable(false);
    fileNameLabel.setMinimumHorizontalScale(1.f);
    fileNameLabel.setInterceptsMouseClicks(false, false);
    channelMenu.setJustificationType(juce::Justification::right);
}

void Document::ReaderLayoutPanel::Channel::Entry::resized()
{
    auto bounds = getLocalBounds();
    thumbLabel.setBounds(bounds.removeFromLeft(26));
    channelMenu.setBounds(bounds.removeFromRight(54));
    fileNameLabel.setBounds(bounds);
}

void Document::ReaderLayoutPanel::Channel::Entry::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
    g.setColour(findColour(Decorator::ColourIds::normalBorderColourId));
    g.fillRect(getLocalBounds().removeFromLeft(26));
}

Document::ReaderLayoutPanel::Channel::Channel(FileInfoPanel& fileInfoPanel, int index, juce::File const& file, int channel, std::unique_ptr<juce::AudioFormatReader> reader)
: mFileInfoPanel(fileInfoPanel)
, mIndex(index)
, mFile(file)
, mReader(std::move(reader))
{
    setInterceptsMouseClicks(false, true);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    addAndMakeVisible(mDecorator);
    
    mEntry.setEnabled(mReader != nullptr);
    mEntry.thumbLabel.setText("#" + juce::String(mIndex + 1), juce::NotificationType::dontSendNotification);
    mEntry.fileNameLabel.setText(mFile.getFileName(), juce::NotificationType::dontSendNotification);
    mEntry.channelMenu.setTextWhenNoChoicesAvailable(juce::String(channel));
    if(mReader != nullptr)
    {
        if(mReader->numChannels > 1u)
        {
            mEntry.channelMenu.addItem(juce::translate("Mono"), -1);
        }
        for(auto numChannel = 1u; numChannel <= mReader->numChannels; ++numChannel)
        {
            mEntry.channelMenu.addItem(juce::String(numChannel), static_cast<int>(numChannel));
        }
        mEntry.channelMenu.setSelectedId(channel == -1 ? -1 : channel + 1, juce::NotificationType::dontSendNotification);
    }
    mEntry.thumbLabel.addMouseListener(this, true);
    mEntry.channelMenu.onChange = [this]()
    {
        if(onChannelChange != nullptr)
        {
            auto const selectedId = mEntry.channelMenu.getSelectedId();
            onChannelChange(selectedId == -1 ? -1 : selectedId - 1);
        }
    };
    
    setSize(300, 24);
}

void Document::ReaderLayoutPanel::Channel::resized()
{
    mDecorator.setBounds(getLocalBounds().withTrimmedBottom(1));
}

void Document::ReaderLayoutPanel::Channel::mouseMove(juce::MouseEvent const& event)
{
    mEntry.thumbLabel.setMouseCursor(event.mods.isCtrlDown() ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::DraggingHandCursor);
}

void Document::ReaderLayoutPanel::Channel::mouseDown(juce::MouseEvent const& event)
{
    mouseMove(event);
    beginDragAutoRepeat(5);
}

void Document::ReaderLayoutPanel::Channel::mouseDrag(juce::MouseEvent const& event)
{
    mouseMove(event);
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
        dragContainer->startDragging(DraggableTable::createDescription(event, "Channel", juce::String(mIndex), getHeight()), this, snapshot, true, &p, &event.source);
    }
}

void Document::ReaderLayoutPanel::Channel::mouseUp(juce::MouseEvent const& event)
{
    mouseMove(event);
}

bool Document::ReaderLayoutPanel::Channel::keyPressed(juce::KeyPress const& key)
{
    if(key.isKeyCurrentlyDown(juce::KeyPress::deleteKey) || key.isKeyCurrentlyDown(juce::KeyPress::backspaceKey))
    {
        if(onDelete != nullptr)
        {
            onDelete();
            return true;
        }
    }
    return false;
}

void Document::ReaderLayoutPanel::Channel::focusGained(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    if(hasKeyboardFocus(true))
    {
        mFileInfoPanel.setAudioFormatReader(mFile, mReader.get());
        mDecorator.setHighlighted(true);
    }
    else
    {
        mDecorator.setHighlighted(false);
    }
}

void Document::ReaderLayoutPanel::Channel::focusLost(juce::Component::FocusChangeType cause)
{
    focusGained(cause);
}

void Document::ReaderLayoutPanel::Channel::focusOfChildComponentChanged(juce::Component::FocusChangeType cause)
{
    focusGained(cause);
}

Document::ReaderLayoutPanel::ReaderLayoutPanel(Director& director)
: FloatingWindowContainer("Audio Reader Layout", *this)
, mDirector(director)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                bool allReadersValid = true;
                bool allSampleRatesValid = true;
                double currentSampleRate = 0.0;
                using ComponentRef = DraggableTable::ComponentRef;
                decltype(mChannels) channelComponents;
                std::vector<ComponentRef> contents;
                auto const channels = acsr.getAttr<AttrType::reader>();
                for(auto const& channel : channels)
                {
                    auto reader = std::unique_ptr<juce::AudioFormatReader>(mAudioFormatManager.createReaderFor(channel.file));
                    if(reader != nullptr)
                    {
                        currentSampleRate = currentSampleRate > 0.0 ? currentSampleRate : reader->sampleRate;
                        allSampleRatesValid = std::abs(currentSampleRate - reader->sampleRate) > std::numeric_limits<double>::epsilon() ? false : allSampleRatesValid;
                    }
                    else
                    {
                        allReadersValid = false;
                    }
                    auto const index = static_cast<int>(contents.size());
                    if(index == 0)
                    {
                        mFileInfoPanel.setAudioFormatReader(channel.file, reader.get());
                    }
                    auto channelComponent = std::make_unique<Channel>(mFileInfoPanel, contents.size(), channel.file, channel.channel, std::move(reader));
                    if(channelComponent != nullptr)
                    {
                        channelComponent->onDelete = [this, index]()
                        {
                            auto readerLayout = mAccessor.getAttr<AttrType::reader>();
                            anlWeakAssert(static_cast<size_t>(index) < readerLayout.size());
                            if(static_cast<size_t>(index) >= readerLayout.size())
                            {
                                return;
                            }
                            readerLayout.erase(readerLayout.begin() + static_cast<long>(index));
                            mDirector.startAction();
                            mAccessor.setAttr<AttrType::reader>(readerLayout, NotificationType::synchronous);
                            mDirector.endAction(ActionState::newTransaction, juce::translate("Remove Audio Files"));
                        };

                        channelComponent->onChannelChange = [this, index](int c)
                        {
                            auto readerLayout = mAccessor.getAttr<AttrType::reader>();
                            anlWeakAssert(static_cast<size_t>(index) < readerLayout.size());
                            if(static_cast<size_t>(index) >= readerLayout.size())
                            {
                                return;
                            }
                            readerLayout[static_cast<size_t>(index)].channel = c;
                            mDirector.startAction();
                            mAccessor.setAttr<AttrType::reader>(readerLayout, NotificationType::synchronous);
                            mDirector.endAction(ActionState::newTransaction, juce::translate("Change Audio Channel"));
                        };

                        contents.push_back(*channelComponent.get());
                        channelComponents.push_back(std::move(channelComponent));
                    }
                }
                mDraggableTable.setComponents(contents);
                mChannels = std::move(channelComponents);
                focusOfChildComponentChanged(juce::Component::FocusChangeType::focusChangedDirectly);
                if(mChannels.empty())
                {
                    mFileInfoPanel.setAudioFormatReader(juce::File{}, nullptr);
                }
                
                mAlertLabel.setVisible(!allReadersValid || !allSampleRatesValid);
                if(!allReadersValid && allSampleRatesValid)
                {
                    mAlertLabel.setText(juce::translate("One or more audio file cannot be allocated and the sample rates of the audio files are not consistent."), juce::NotificationType::dontSendNotification);
                }
                else if(!allReadersValid)
                {
                    mAlertLabel.setText(juce::translate("One or more audio file cannot be allocated."), juce::NotificationType::dontSendNotification);
                }
                else if(!allSampleRatesValid)
                {
                    mAlertLabel.setText(juce::translate("The sample rates of the audio files are not consistent."), juce::NotificationType::dontSendNotification);
                }
                mAlertButton.setTooltip(mAlertLabel.getText());
                mAlertButton.setVisible(mAlertLabel.isVisible());
                resized();
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
                break;
        }
    };

    mBoundsListener.onComponentResized = [this](juce::Component&)
    {
        resized();
    };

    mDraggableTable.onComponentDropped = [this](juce::String const& identifier, size_t index, bool copy)
    {
        auto readerLayout = mAccessor.getAttr<AttrType::reader>();
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
        mDirector.startAction();
        mAccessor.setAttr<AttrType::reader>(readerLayout, NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Move Audio Files"));
    };

    mAddButton.onClick = [this]()
    {
        juce::FileChooser fc("Load audio files...", {}, mAudioFormatManager.getWildcardForAllFormats());
        if(!fc.browseForMultipleFilesToOpen())
        {
            return;
        }
        auto readerLayout = mAccessor.getAttr<AttrType::reader>();
        for(auto const& result : fc.getResults())
        {
            readerLayout.push_back({result});
        }
        mDirector.startAction();
        mAccessor.setAttr<AttrType::reader>(readerLayout, NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Insert Audio Files"));
    };

    mAddLabel.setInterceptsMouseClicks(false, false);
    mAddLabel.setText(juce::translate("Insert audio files..."), juce::NotificationType::dontSendNotification);
    mAddLabel.setTooltip(juce::translate("Add audio files to the document..."));
    mAddButton.setTooltip(juce::translate("Add audio files to the document..."));
    
    mBoundsListener.attachTo(mDraggableTable);
    mViewport.setViewedComponent(&mDraggableTable, false);
    mViewport.setScrollBarsShown(true, false, false, false);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mAddButton);
    addAndMakeVisible(mAddLabel);
    addChildComponent(mAlertButton);
    addChildComponent(mAlertLabel);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mFileInfoPanel);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(300, 400);
}

Document::ReaderLayoutPanel::~ReaderLayoutPanel()
{
    mBoundsListener.detachFrom(mDraggableTable);
    mAccessor.removeListener(mListener);
}

void Document::ReaderLayoutPanel::resized()
{
    auto bounds = getLocalBounds();
    mFileInfoPanel.setBounds(bounds.removeFromBottom(168));
    mSeparator.setBounds(bounds.removeFromBottom(1));
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

void Document::ReaderLayoutPanel::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.setColour(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
        g.fillRect(0, 0, getWidth(), mSeparator.getY());
    }
}

void Document::ReaderLayoutPanel::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mAddButton, IconManager::IconType::plus);
        laf->setButtonIcon(mAlertButton, IconManager::IconType::alert);
    }
}

void Document::ReaderLayoutPanel::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

bool Document::ReaderLayoutPanel::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    for(auto const& result : files)
    {
        juce::File const file(result);
        if(wildcard.contains(file.getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Document::ReaderLayoutPanel::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    mIsDragging = true;
    repaint();
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    for(auto const& result : files)
    {
        juce::File const file(result);
        if(wildcard.contains(file.getFileExtension()))
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(mAudioFormatManager.createReaderFor(file));
            mFileInfoPanel.setAudioFormatReader(file, reader.get());
            return;
        }
    }
}

void Document::ReaderLayoutPanel::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    focusOfChildComponentChanged(juce::Component::FocusChangeType::focusChangedDirectly);
    mIsDragging = false;
    repaint();
}

void Document::ReaderLayoutPanel::filesDropped(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    auto const wildcard = mAudioFormatManager.getWildcardForAllFormats();
    auto readerLayout = mAccessor.getAttr<AttrType::reader>();
    for(auto const& result : files)
    {
        juce::File const file(result);
        if(wildcard.contains(file.getFileExtension()))
        {
            readerLayout.push_back({result});
        }
    }
    mDirector.startAction();
    mAccessor.setAttr<AttrType::reader>(readerLayout, NotificationType::synchronous);
    mDirector.endAction(ActionState::newTransaction, juce::translate("Insert Audio Files"));
    focusOfChildComponentChanged(juce::Component::FocusChangeType::focusChangedDirectly);
    mIsDragging = false;
    repaint();
}

ANALYSE_FILE_END
