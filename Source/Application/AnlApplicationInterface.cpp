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
            case AttrType::recentlyOpenedFilesList:
            {
                mListBox.updateContent();
                mListBox.repaint();
            }
            break;
            case AttrType::windowState:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
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
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f));
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
        commandManager.invokeDirectly(CommandIDs::documentOpen, true);
    };
    mAddTrackButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::editNewTrack, true);
    };
    mLoadTemplateButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::editLoadTemplate, true);
    };
    mAdaptationButton.onClick = [this]()
    {
        Instance::get().getApplicationAccessor().setAttr<AttrType::adaptationToSampleRate>(mAdaptationButton.getToggleState(), NotificationType::synchronous);
    };

    mLoadFileInfo.setText(juce::translate("or\nDrag & Drop\n(Document/Audio)"), juce::NotificationType::dontSendNotification);
    mLoadFileInfo.setJustificationType(juce::Justification::centredTop);

    mLoadFileWildcard.setText(juce::translate("Document: DOCWILDCARD\nAudio: AUDIOWILDCARD").replace("DOCWILDCARD", Instance::getDocumentFileWildCard()).replace("AUDIOWILDCARD", "*.aac,*.aiff,*.aif,*.flac,*.m4a,*.mp3,*.ogg,*.wav,*.wma"), juce::NotificationType::dontSendNotification);
    mLoadFileWildcard.setJustificationType(juce::Justification::bottomLeft);

    mAddTrackInfo.setText(juce::translate("Insert an analysis plugin as a new track"), juce::NotificationType::dontSendNotification);
    mAddTrackInfo.setJustificationType(juce::Justification::centredTop);

    mLoadTemplateInfo.setText(juce::translate("Use an existing document as a template"), juce::NotificationType::dontSendNotification);
    mLoadTemplateInfo.setJustificationType(juce::Justification::centredTop);

    mAdaptationInfo.setText(juce::translate("Adapt the block size and the step size of the analyzes to the sample rate"), juce::NotificationType::dontSendNotification);

    addAndMakeVisible(mSeparatorVertical);
    addAndMakeVisible(mSeparatorTop);
    addAndMakeVisible(mSelectRecentDocument);
    addAndMakeVisible(mFileTable);

    mFileTable.onFileSelected = [](juce::File const& file)
    {
        auto& documentAcsr = Instance::get().getDocumentAccessor();
        auto const documentHasFile = !documentAcsr.getAttr<Document::AttrType::reader>().empty();
        if(!documentHasFile)
        {
            Instance::get().openDocumentFile(file);
        }
        else
        {
            auto& documentDir = Instance::get().getDocumentDirector();
            documentDir.startAction();
            auto const adaptation = Instance::get().getApplicationAccessor().getAttr<AttrType::adaptationToSampleRate>();
            auto const results = Instance::get().getDocumentFileBased().loadTemplate(file, adaptation);
            if(results.failed())
            {
                AlertWindow::showMessage(AlertWindow::MessageType::warning, "Failed to load template!", results.getErrorMessage());
                documentDir.endAction(ActionState::abort);
            }
            else
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Load template"));
            }
        }
    };

    mDocumentListener.onAttrChanged = [this](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Document::AttrType::reader:
            {
                updateState();
            }
            break;
            case Document::AttrType::layout:
            case Document::AttrType::viewport:
            case Document::AttrType::path:
            case Document::AttrType::grid:
            case Document::AttrType::samplerate:
                break;
        }
    };

    mDocumentListener.onAccessorErased = mDocumentListener.onAccessorInserted = [this](Document::Accessor const& acsr, Document::AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, type, index);
        updateState();
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::autoLoadConvertedFile:
                break;
            case AttrType::adaptationToSampleRate:
            {
                mAdaptationButton.setToggleState(acsr.getAttr<AttrType::adaptationToSampleRate>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);

    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, NotificationType::synchronous);
    commandManager.addListener(this);
}

Application::Interface::Loader::~Loader()
{
    Instance::get().getApplicationCommandManager().removeListener(this);
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);

    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::Interface::Loader::setDragging(bool state)
{
    mIsDragging = state;
    using ButtonState = juce::Button::ButtonState;
    mLoadFileButton.setState(state ? ButtonState::buttonOver : ButtonState::buttonNormal);
    repaint();
}

void Application::Interface::Loader::updateState()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasFiles = !documentAccessor.getAttr<Document::AttrType::reader>().empty();
    auto const documentHasTrackOrGroup = documentAccessor.getNumAcsrs<Document::AcsrType::tracks>() > 0_z || documentAccessor.getNumAcsrs<Document::AcsrType::groups>() > 0_z;

    removeChildComponent(&mLoadFileButton);
    removeChildComponent(&mLoadFileInfo);
    removeChildComponent(&mLoadFileWildcard);
    removeChildComponent(&mAddTrackButton);
    removeChildComponent(&mAddTrackInfo);
    removeChildComponent(&mLoadTemplateButton);
    removeChildComponent(&mLoadTemplateInfo);
    removeChildComponent(&mSeparatorBottom);
    removeChildComponent(&mAdaptationButton);
    removeChildComponent(&mAdaptationInfo);
    setVisible(!documentHasTrackOrGroup);
    if(!documentHasFiles)
    {
        addAndMakeVisible(mLoadFileButton);
        addAndMakeVisible(mLoadFileInfo);
        addAndMakeVisible(mLoadFileWildcard);
        mSelectRecentDocument.setText(juce::translate("Load a recent document"), juce::NotificationType::dontSendNotification);
    }
    else if(!documentHasTrackOrGroup)
    {
        addAndMakeVisible(mAddTrackButton);
        addAndMakeVisible(mAddTrackInfo);
        addAndMakeVisible(mLoadTemplateButton);
        addAndMakeVisible(mLoadTemplateInfo);
        addAndMakeVisible(mSeparatorBottom);
        addAndMakeVisible(mAdaptationButton);
        addAndMakeVisible(mAdaptationInfo);
        mSelectRecentDocument.setText(juce::translate("Use a document as a template"), juce::NotificationType::dontSendNotification);
    }
    resized();
}

void Application::Interface::Loader::resized()
{
    auto bounds = getLocalBounds();
    {
        auto rightBounds = bounds.removeFromRight(220);
        mSelectRecentDocument.setBounds(rightBounds.removeFromTop(32));
        mSeparatorTop.setBounds(rightBounds.removeFromTop(1));
        if(mAdaptationButton.isVisible())
        {
            auto bottomBounds = rightBounds.removeFromBottom(64);
            mAdaptationButton.setBounds(bottomBounds.removeFromLeft(24).withSizeKeepingCentre(18, 18));
            mAdaptationInfo.setBounds(bottomBounds);
            mSeparatorBottom.setBounds(rightBounds.removeFromBottom(1));
        }
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
            mLoadTemplateInfo.setBounds(centerRightBounds);
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
    mLoadFileButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::documentOpen));
    mAddTrackButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::editNewTrack));
    mLoadTemplateButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::editLoadTemplate));
}

Application::Interface::Interface()
: mDocumentSection(Instance::get().getDocumentDirector())
{
    mComponentListener.onComponentVisibilityChanged = [this](juce::Component& component)
    {
        anlStrongAssert(&component == &mLoader);
        if(&component != &mLoader)
        {
            return;
        }
        mLoaderDecorator.setVisible(mLoader.isVisible());
        mDocumentSection.setEnabled(!mLoader.isVisible());
    };

    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mLoaderDecorator);
    addAndMakeVisible(mToolTipSeparator);
    addAndMakeVisible(mToolTipDisplay);
    mComponentListener.attachTo(mLoader);

    mDocumentSection.tooltipButton.setClickingTogglesState(true);
    mDocumentSection.tooltipButton.setTooltip(Instance::get().getApplicationCommandManager().getDescriptionOfCommand(CommandTarget::CommandIDs::viewInfoBubble));
    mDocumentSection.tooltipButton.onClick = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandTarget::CommandIDs::viewInfoBubble, true);
    };
    mDocumentSection.onSaveButtonClicked = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandTarget::CommandIDs::documentSave, true);
    };
    mDocumentSection.onNewTrackButtonClicked = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandTarget::CommandIDs::editNewTrack, true);
    };
    mDocumentSection.onNewGroupButtonClicked = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandTarget::CommandIDs::editNewGroup, true);
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
                break;
            case AttrType::showInfoBubble:
            {
                mDocumentSection.showBubbleInfo(acsr.getAttr<AttrType::showInfoBubble>());
                mDocumentSection.tooltipButton.setToggleState(acsr.getAttr<AttrType::showInfoBubble>(), juce::NotificationType::dontSendNotification);
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
    mComponentListener.detachFrom(mLoader);
}

void Application::Interface::resized()
{
    auto bounds = getLocalBounds();
    auto bottom = bounds.removeFromBottom(22);
    mToolTipDisplay.setBounds(bottom);
    mToolTipSeparator.setBounds(bounds.removeFromBottom(1));
    mDocumentSection.setBounds(bounds);
    auto const loaderBounds = bounds.withSizeKeepingCentre(800, 600);
    mLoaderDecorator.setBounds(loaderBounds.withY(std::max(loaderBounds.getY(), 80)));
}

bool Application::Interface::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasAudioFiles = !documentAccessor.getAttr<Document::AttrType::reader>().empty();

    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats();
    auto const documentWildcard = Instance::getDocumentFileWildCard();
    auto const fileWildcard = documentWildcard + (documentHasAudioFiles ? "" : (";" + audioFormatWildcard));
    for(auto const& fileName : files)
    {
        if(fileWildcard.contains(juce::File(fileName).getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Application::Interface::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files);
    mLoader.setDragging(mDocumentSection.getBounds().contains(x, y));
}

void Application::Interface::fileDragMove(juce::StringArray const& files, int x, int y)
{
    fileDragEnter(files, x, y);
}

void Application::Interface::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    mLoader.setDragging(false);
}

void Application::Interface::filesDropped(juce::StringArray const& files, int x, int y)
{
    mLoader.setDragging(false);
    if(!mDocumentSection.getBounds().contains(x, y))
    {
        return;
    }

    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasAudioFiles = !documentAccessor.getAttr<Document::AttrType::reader>().empty();

    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats();
    auto const documentWildcard = Instance::getDocumentFileWildCard();
    auto const fileWildcard = documentWildcard + (documentHasAudioFiles ? "" : (";" + audioFormatWildcard));
    auto getFiles = [&]()
    {
        std::vector<juce::File> loadedFiles;
        for(auto const& fileName : files)
        {
            juce::File const file(fileName);
            if(fileWildcard.contains(file.getFileExtension()))
            {
                loadedFiles.push_back(file);
            }
        }
        return loadedFiles;
    };
    auto const loadedFiles = getFiles();
    if(loadedFiles.empty())
    {
        return;
    }
    Instance::get().openFiles(loadedFiles);
}

void Application::Interface::moveKeyboardFocusTo(juce::String const& identifier)
{
    mDocumentSection.moveKeyboardFocusTo(identifier);
}

juce::Rectangle<int> Application::Interface::getPlotBounds(juce::String const& identifier) const
{
    return mDocumentSection.getPlotBounds(identifier);
}

ANALYSE_FILE_END
