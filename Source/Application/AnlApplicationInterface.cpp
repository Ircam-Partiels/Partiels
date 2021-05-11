#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Loader::FileTable::FileTable()
{
    addAndMakeVisible(mListBox);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::windowState:
                break;
            case AttrType::recentlyOpenedFilesList:
            {
                mListBox.updateContent();
                mListBox.repaint();
            }
            break;
            case AttrType::currentDocumentFile:
                break;
            case AttrType::colourMode:
                break;
            case AttrType::showInfoBubble:
                break;
        }
    };

    auto& accessor = Instance::get().getApplicationAccessor();
    accessor.addListener(mListener, NotificationType::synchronous);
}

Application::Interface::Loader::FileTable::~FileTable()
{
    auto& accessor = Instance::get().getApplicationAccessor();
    accessor.removeListener(mListener);
}

void Application::Interface::Loader::FileTable::resized()
{
    mListBox.setBounds(getLocalBounds());
}

int Application::Interface::Loader::FileTable::getNumRows()
{
    auto const& accessor = Instance::get().getApplicationAccessor();
    return static_cast<int>(accessor.getAttr<AttrType::recentlyOpenedFilesList>().size());
}

void Application::Interface::Loader::FileTable::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto defaultColour = mListBox.findColour(juce::ListBox::backgroundColourId);
    const auto selectedColour = defaultColour.interpolatedWith(mListBox.findColour(juce::ListBox::textColourId), 0.5f);
    g.fillAll(rowIsSelected ? selectedColour : defaultColour);

    auto const& accessor = Instance::get().getApplicationAccessor();
    auto const& files = accessor.getAttr<AttrType::recentlyOpenedFilesList>();
    if(rowNumber >= static_cast<int>(files.size()))
    {
        return;
    }

    auto const index = static_cast<size_t>(rowNumber);

    auto const fileName = files[index].getFileNameWithoutExtension();
    auto const hasDuplicate = std::count_if(files.cbegin(), files.cend(), [&](auto const file)
                                            {
                                                return fileName == file.getFileNameWithoutExtension();
                                            }) > 1l;
    auto const text = fileName + (hasDuplicate ? " (" + files[index].getParentDirectory().getFileName() + ")" : "");

    g.setColour(mListBox.findColour(juce::ListBox::textColourId));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f, juce::Font::bold));
    g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 1.f);
}

void Application::Interface::Loader::FileTable::returnKeyPressed(int lastRowSelected)
{
    auto const& accessor = Instance::get().getApplicationAccessor();
    auto const& files = accessor.getAttr<AttrType::recentlyOpenedFilesList>();
    if(lastRowSelected >= static_cast<int>(files.size()))
    {
        return;
    }
    auto const index = static_cast<size_t>(lastRowSelected);
    if(onFileSelected != nullptr)
    {
        onFileSelected(files[index]);
    }
}

void Application::Interface::Loader::FileTable::listBoxItemDoubleClicked(int row, juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    returnKeyPressed(row);
}

Application::Interface::Loader::Loader()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::DocumentOpen, true);
    };
    mAddTrackButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::EditNewTrack, true);
    };
    mLoadTemplateButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::EditLoadTemplate, true);
    };
    mLoadFileInfo.setText(juce::translate("or\nDrag & Drop\n(Document/Audio)"), juce::NotificationType::dontSendNotification);
    mLoadFileInfo.setJustificationType(juce::Justification::centredTop);

    mLoadFileWildcard.setText(juce::translate("Document: DOCWILDCARD\nAudio: AUDIOWILDCARD").replace("DOCWILDCARD", Instance::getFileWildCard()).replace("AUDIOWILDCARD", "*.aac,*.aiff,*.aif,*.flac,*.m4a,*.mp3,*.ogg,*.wav,*.wma"), juce::NotificationType::dontSendNotification);
    mLoadFileWildcard.setJustificationType(juce::Justification::bottomLeft);

    mAddTrackInfo.setText(juce::translate("Insert an analysis plugin as a new track"), juce::NotificationType::dontSendNotification);
    mAddTrackInfo.setJustificationType(juce::Justification::centredTop);

    mLoadTemplateInfo.setText(juce::translate("Use an existing document as template"), juce::NotificationType::dontSendNotification);
    mLoadTemplateInfo.setJustificationType(juce::Justification::centredTop);

    addAndMakeVisible(mSeparatorVertical);
    addAndMakeVisible(mSeparatorHorizontal);
    addAndMakeVisible(mSelectRecentDocument);
    addAndMakeVisible(mFileTable);

    mFileTable.onFileSelected = [](juce::File const& file)
    {
        auto& documentAcsr = Instance::get().getDocumentAccessor();
        auto const documentHasFile = documentAcsr.getAttr<Document::AttrType::file>().existsAsFile();
        if(!documentHasFile)
        {
            Instance::get().openFile(file);
        }
        else
        {
            Document::Accessor copyAcsr;
            juce::UndoManager copyUndoManager;
            Document::Director copyDirector{copyAcsr, Instance::get().getAudioFormatManager(), copyUndoManager};
            Document::FileBased copyFileBased{copyAcsr, copyDirector, Instance::getFileExtension(), Instance::getFileWildCard(), "Open a document", "Save the document"};
            if(copyFileBased.loadFrom(file, true).failed())
            {
                return;
            }

            copyAcsr.setAttr<Document::AttrType::file>(documentAcsr.getAttr<Document::AttrType::file>(), NotificationType::synchronous);

            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            documentAcsr.copyFrom(copyAcsr, NotificationType::synchronous);
            documentDir.endAction("Load template", ActionState::apply);
        }
    };

    mDocumentListener.onAttrChanged = [this](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Document::AttrType::file:
            {
                updateState();
            }
            break;
            case Document::AttrType::layout:
                break;
        }
    };

    mDocumentListener.onAccessorErased = mDocumentListener.onAccessorInserted = [this](Document::Accessor const& acsr, Document::AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, type, index);
        updateState();
    };

    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, NotificationType::synchronous);
    commandManager.addListener(this);
}

Application::Interface::Loader::~Loader()
{
    Instance::get().getApplicationCommandManager().removeListener(this);
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);
}

void Application::Interface::Loader::updateState()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasFile = documentAccessor.getAttr<Document::AttrType::file>().existsAsFile();
    auto const documentHasTrackOrGroup = documentAccessor.getNumAcsrs<Document::AcsrType::tracks>() > 0_z || documentAccessor.getNumAcsrs<Document::AcsrType::groups>() > 0_z;

    removeChildComponent(&mLoadFileButton);
    removeChildComponent(&mLoadFileInfo);
    removeChildComponent(&mLoadFileWildcard);
    removeChildComponent(&mAddTrackButton);
    removeChildComponent(&mAddTrackInfo);
    removeChildComponent(&mLoadTemplateButton);
    removeChildComponent(&mLoadTemplateInfo);
    setVisible(!documentHasFile || !documentHasTrackOrGroup);
    if(!documentHasFile)
    {
        addAndMakeVisible(mLoadFileButton);
        addAndMakeVisible(mLoadFileInfo);
        addAndMakeVisible(mLoadFileWildcard);
    }
    else if(!documentHasTrackOrGroup)
    {
        addAndMakeVisible(mAddTrackButton);
        addAndMakeVisible(mAddTrackInfo);
        addAndMakeVisible(mLoadTemplateButton);
        addAndMakeVisible(mLoadTemplateInfo);
    }
}

void Application::Interface::Loader::resized()
{
    auto bounds = getLocalBounds();
    {
        auto rightBounds = bounds.removeFromRight(220);
        mSelectRecentDocument.setBounds(rightBounds.removeFromTop(32));
        mSeparatorHorizontal.setBounds(rightBounds.removeFromTop(1));
        mFileTable.setBounds(rightBounds);
        mSeparatorVertical.setBounds(bounds.removeFromRight(1));
    }
    {
        auto centerBounds = bounds.withSizeKeepingCentre(160, 82);
        mLoadFileButton.setBounds(centerBounds.removeFromTop(32));
        mLoadFileInfo.setBounds(centerBounds);
        auto bottomBounds = bounds;
        mLoadFileWildcard.setBounds(bottomBounds.removeFromBottom(64));
    }
    {
        auto centerBounds = bounds.withSizeKeepingCentre(402, 82);
        {
            auto centerLeftBounds = centerBounds.removeFromLeft(200);
            mAddTrackButton.setBounds(centerLeftBounds.removeFromTop(32));
            mAddTrackInfo.setBounds(centerLeftBounds);
        }
        {
            auto centerRightBounds = centerBounds.withTrimmedLeft(2);
            mLoadTemplateButton.setBounds(centerRightBounds.removeFromTop(32));
            mLoadTemplateInfo.setBounds(centerRightBounds.removeFromTop(32));
        }
    }
}

void Application::Interface::Loader::paint(juce::Graphics& g)
{
    g.setColour(mIsDragging ? findColour(juce::TextButton::ColourIds::buttonColourId) : findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().withTrimmedRight(222).toFloat(), 2.0f);
}

void Application::Interface::Loader::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    juce::ignoreUnused(info);
}

void Application::Interface::Loader::applicationCommandListChanged()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::DocumentOpen));
    mAddTrackButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::EditNewTrack));
    mLoadTemplateButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::EditLoadTemplate));
}

bool Application::Interface::Loader::isInterestedInFileDrag(juce::StringArray const& files)
{
    if(!mLoadFileButton.isVisible())
    {
        return false;
    }
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    for(auto const& fileName : files)
    {
        if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Application::Interface::Loader::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    mIsDragging = true;
    juce::ignoreUnused(files, x, y);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonOver);
    repaint();
}

void Application::Interface::Loader::fileDragExit(juce::StringArray const& files)
{
    mIsDragging = false;
    juce::ignoreUnused(files);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonNormal);
    repaint();
}

void Application::Interface::Loader::filesDropped(juce::StringArray const& files, int x, int y)
{
    mIsDragging = false;
    juce::ignoreUnused(x, y);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonNormal);
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    auto getFile = [&]()
    {
        for(auto const& fileName : files)
        {
            if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
            {
                return juce::File(fileName);
            }
        }
        return juce::File();
    };
    auto const file = getFile();
    if(file == juce::File())
    {
        return;
    }
    Instance::get().openFile(file);
    repaint();
}

Application::Interface::Interface()
: mDocumentSection(Instance::get().getDocumentDirector())
{
    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mLoaderDecorator);
    addAndMakeVisible(mToolTipSeparator);
    addAndMakeVisible(mToolTipDisplay);
    addAndMakeVisible(mTooltipButton);
    mLoader.addComponentListener(this);

    mTooltipButton.setClickingTogglesState(true);
    mTooltipButton.onClick = [&]()
    {
        auto& accessor = Instance::get().getApplicationAccessor();
        accessor.setAttr<AttrType::showInfoBubble>(mTooltipButton.getToggleState(), NotificationType::synchronous);
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::windowState:
                break;
            case AttrType::recentlyOpenedFilesList:
                break;
            case AttrType::currentDocumentFile:
                break;
            case AttrType::colourMode:
                break;
            case AttrType::showInfoBubble:
            {
                mDocumentSection.showBubbleInfo(acsr.getAttr<AttrType::showInfoBubble>());
                mTooltipButton.setToggleState(acsr.getAttr<AttrType::showInfoBubble>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };
    auto& accessor = Instance::get().getApplicationAccessor();
    accessor.addListener(mListener, NotificationType::synchronous);
}

Application::Interface::~Interface()
{
    auto& accessor = Instance::get().getApplicationAccessor();
    accessor.removeListener(mListener);
    mLoader.removeComponentListener(this);
}

void Application::Interface::resized()
{
    auto bounds = getLocalBounds();
    auto bottom = bounds.removeFromBottom(24);
    mTooltipButton.setBounds(bottom.removeFromRight(24).withSizeKeepingCentre(20, 20));
    mToolTipDisplay.setBounds(bottom);
    mToolTipSeparator.setBounds(bounds.removeFromBottom(1));
    mDocumentSection.setBounds(bounds);
    mLoaderDecorator.setBounds(bounds.withSizeKeepingCentre(800, 600));
}

void Application::Interface::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mTooltipButton, IconManager::IconType::conversation);
    }
}

void Application::Interface::moveKeyboardFocusTo(juce::String const& identifier)
{
    mDocumentSection.moveKeyboardFocusTo(identifier);
}

juce::Rectangle<int> Application::Interface::getPlotBounds(juce::String const& identifier) const
{
    return mDocumentSection.getPlotBounds(identifier);
}

void Application::Interface::componentVisibilityChanged(juce::Component& component)
{
    anlStrongAssert(&component == &mLoader);
    if(&component != &mLoader)
    {
        return;
    }
    mLoaderDecorator.setVisible(mLoader.isVisible());
    mDocumentSection.setEnabled(!mLoader.isVisible());
}

ANALYSE_FILE_END
