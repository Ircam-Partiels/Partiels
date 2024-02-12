#include "AnlApplicationLoader.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::LoaderContent::FileTable::FileTable()
{
    addAndMakeVisible(mListBox);

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
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
            case AttrType::defaultTemplateFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::routingMatrix:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
}

Application::LoaderContent::FileTable::~FileTable()
{
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::LoaderContent::FileTable::resized()
{
    mListBox.setBounds(getLocalBounds());
}

int Application::LoaderContent::FileTable::getNumRows()
{
    return static_cast<int>(Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>().size());
}

void Application::LoaderContent::FileTable::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto defaultColour = mListBox.findColour(juce::ListBox::backgroundColourId);
    const auto selectedColour = defaultColour.interpolatedWith(mListBox.findColour(juce::ListBox::textColourId), 0.5f);
    g.fillAll(rowIsSelected ? selectedColour : defaultColour);

    auto const& files = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
    if(rowNumber >= static_cast<int>(files.size()))
    {
        return;
    }

    auto const index = static_cast<size_t>(rowNumber);

    auto const fileName = files.at(index).getFileNameWithoutExtension();
    auto const hasDuplicate = std::count_if(files.cbegin(), files.cend(), [&](auto const file)
                                            {
                                                return fileName == file.getFileNameWithoutExtension();
                                            }) > 1l;
    auto const text = fileName + (hasDuplicate ? " (" + files[index].getParentDirectory().getFileName() + ")" : "");

    g.setColour(mListBox.findColour(juce::ListBox::textColourId));
    g.setFont(juce::Font(static_cast<float>(height) * 0.7f));
    g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 1.f);
}

void Application::LoaderContent::FileTable::returnKeyPressed(int lastRowSelected)
{
    auto const& files = Instance::get().getApplicationAccessor().getAttr<AttrType::recentlyOpenedFilesList>();
    if(lastRowSelected >= static_cast<int>(files.size()))
    {
        return;
    }
    auto const index = static_cast<size_t>(lastRowSelected);
    if(onFileSelected != nullptr)
    {
        onFileSelected(files.at(index));
    }
}

void Application::LoaderContent::FileTable::listBoxItemDoubleClicked(int row, [[maybe_unused]] juce::MouseEvent const& event)
{
    returnKeyPressed(row);
}

Application::LoaderContent::LoaderContent()
{
    using CommandIDs = CommandTarget::CommandIDs;
    mLoadFileButton.onClick = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::documentOpen, true);
    };
    mAddTrackButton.onClick = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::editNewTrack, true);
    };
    mLoadTemplateButton.onClick = []()
    {
        Instance::get().getApplicationCommandManager().invokeDirectly(CommandIDs::editLoadTemplate, true);
    };
    mAdaptationButton.onClick = [this]()
    {
        Instance::get().getApplicationAccessor().setAttr<AttrType::adaptationToSampleRate>(mAdaptationButton.getToggleState(), NotificationType::synchronous);
    };

    mLoadFileInfo.setText(juce::translate("or\nDrag & Drop\n(Document/Audio)"), juce::NotificationType::dontSendNotification);
    mLoadFileInfo.setJustificationType(juce::Justification::centredTop);

    mLoadFileWildcard.setText(juce::translate("Document: DOCWILDCARD\nAudio: AUDIOWILDCARD").replace("DOCWILDCARD", Instance::getWildCardForDocumentFile()).replace("AUDIOWILDCARD", "*.aac,*.aiff,*.aif,*.flac,*.m4a,*.mp3,*.ogg,*.wav,*.wma"), juce::NotificationType::dontSendNotification);
    mLoadFileWildcard.setJustificationType(juce::Justification::bottomLeft);

    mAddTrackInfo.setText(juce::translate("Insert an analysis plugin as a new track"), juce::NotificationType::dontSendNotification);
    mAddTrackInfo.setJustificationType(juce::Justification::centredTop);

    mLoadTemplateInfo.setText(juce::translate("Use an existing document as a template"), juce::NotificationType::dontSendNotification);
    mLoadTemplateInfo.setJustificationType(juce::Justification::centredTop);

    addAndMakeVisible(mSeparatorVertical);
    addAndMakeVisible(mSeparatorTop);
    addAndMakeVisible(mSelectRecentDocument);
    addAndMakeVisible(mFileTable);
    addAndMakeVisible(mLoadFileButton);
    addAndMakeVisible(mLoadFileInfo);
    addAndMakeVisible(mLoadFileWildcard);
    addAndMakeVisible(mAddTrackButton);
    addAndMakeVisible(mAddTrackInfo);
    addAndMakeVisible(mLoadTemplateButton);
    addAndMakeVisible(mLoadTemplateInfo);
    addAndMakeVisible(mTemplateMenu);
    addAndMakeVisible(mAdaptationButton);
    addAndMakeVisible(mBottomInfo);
    addAndMakeVisible(mSeparatorBottom);
    setSize(800, 600);

    mTemplateMenu.addItemList({juce::translate("None"), juce::translate("Factory"), juce::translate("Select...")}, 1);
    mTemplateMenu.onChange = [this]()
    {
        switch(mTemplateMenu.getSelectedId())
        {
            case 1:
                Instance::get().getApplicationAccessor().setAttr<AttrType::defaultTemplateFile>(juce::File(), NotificationType::synchronous);
                break;
            case 2:
                Instance::get().getApplicationAccessor().setAttr<AttrType::defaultTemplateFile>(Accessor::getFactoryTemplateFile(), NotificationType::synchronous);
                break;
            case 3:
            {
                if(auto* window = Instance::get().getWindow())
                {
                    window->getInterface().selectDefaultTemplateFile();
                }
            }
            break;
            default:
                break;
        }
    };

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
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::WarningIcon)
                                         .withTitle(juce::translate("Failed to load template!"))
                                         .withMessage(results.getErrorMessage())
                                         .withButton(juce::translate("Ok"));
                juce::AlertWindow::showAsync(options, nullptr);
                documentDir.endAction(ActionState::abort);
            }
            else
            {
                documentDir.endAction(ActionState::newTransaction, juce::translate("Load template"));
            }
        }
    };

    auto const updateState = [this]()
    {
        auto const isEmpty = Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>().empty();
        mLoadFileButton.setVisible(isEmpty);
        mLoadFileInfo.setVisible(isEmpty);
        mLoadFileWildcard.setVisible(isEmpty);
        mAddTrackButton.setVisible(!isEmpty);
        mAddTrackInfo.setVisible(!isEmpty);
        mLoadTemplateButton.setVisible(!isEmpty);
        mLoadTemplateInfo.setVisible(!isEmpty);
        mTemplateMenu.setVisible(isEmpty);
        mAdaptationButton.setVisible(!isEmpty);
        if(isEmpty)
        {
            mSelectRecentDocument.setText(juce::translate("Load a recent document"), juce::NotificationType::dontSendNotification);
            mBottomInfo.setText(juce::translate("Default template loaded automatically when loading an audio file"), juce::NotificationType::dontSendNotification);
        }
        else
        {
            mSelectRecentDocument.setText(juce::translate("Use a document as a template"), juce::NotificationType::dontSendNotification);
            mBottomInfo.setText(juce::translate("Adapt the block size and the step size of the analyzes to the sample rate"), juce::NotificationType::dontSendNotification);
        }
        resized();
    };

    mDocumentListener.onAttrChanged = [=]([[maybe_unused]] Document::Accessor const& acsr, Document::AttrType attribute)
    {
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
            case Document::AttrType::autoresize:
            case Document::AttrType::samplerate:
            case Document::AttrType::channels:
            case Document::AttrType::editMode:
            case Document::AttrType::drawingState:
                break;
        }
    };

    mDocumentListener.onAccessorErased = mDocumentListener.onAccessorInserted = [=]([[maybe_unused]] Document::Accessor const& acsr, [[maybe_unused]] Document::AcsrType type, [[maybe_unused]] size_t index)
    {
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
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::routingMatrix:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
            case AttrType::adaptationToSampleRate:
            {
                mAdaptationButton.setToggleState(acsr.getAttr<AttrType::adaptationToSampleRate>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::defaultTemplateFile:
            {
                auto const file = acsr.getAttr<AttrType::defaultTemplateFile>();
                if(!file.existsAsFile())
                {
                    mTemplateMenu.setSelectedId(1, juce::NotificationType::dontSendNotification);
                }
                else if(file == Accessor::getFactoryTemplateFile())
                {
                    mTemplateMenu.setSelectedId(2, juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mTemplateMenu.setText(file.getFileNameWithoutExtension(), juce::NotificationType::dontSendNotification);
                }
            }
            break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
    Instance::get().getApplicationCommandManager().addListener(this);
}

Application::LoaderContent::~LoaderContent()
{
    Instance::get().getApplicationCommandManager().removeListener(this);
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::LoaderContent::resized()
{
    auto bounds = getLocalBounds();
    {
        auto rightBounds = bounds.removeFromRight(220);
        mSelectRecentDocument.setBounds(rightBounds.removeFromTop(32));
        mSeparatorTop.setBounds(rightBounds.removeFromTop(1));
        auto bottomBounds = rightBounds.removeFromBottom(64);
        if(mTemplateMenu.isVisible())
        {
            mTemplateMenu.setBounds(bottomBounds.removeFromBottom(24));
        }
        else if(mAdaptationButton.isVisible())
        {
            mAdaptationButton.setBounds(bottomBounds.removeFromLeft(24).withSizeKeepingCentre(18, 18));
        }
        mBottomInfo.setBounds(bottomBounds);
        mSeparatorBottom.setBounds(rightBounds.removeFromBottom(1));
        mFileTable.setBounds(rightBounds);
        mSeparatorVertical.setBounds(bounds.removeFromRight(1));
    }
    {
        auto centerBounds = bounds.withSizeKeepingCentre(std::min(160, bounds.getWidth()), std::min(82, bounds.getHeight()));
        mLoadFileButton.setBounds(centerBounds.removeFromTop(32));
        mLoadFileInfo.setBounds(centerBounds);
        auto bottomBounds = bounds;
        mLoadFileWildcard.setBounds(bottomBounds.removeFromBottom(64));
    }
    {
        auto centerBounds = bounds.withSizeKeepingCentre(std::min(402, bounds.getWidth()), std::min(82, bounds.getHeight()));
        auto const buttonWidth = (centerBounds.getWidth() - 2) / 2;
        auto centerLeftBounds = centerBounds.removeFromLeft(buttonWidth);
        mAddTrackButton.setBounds(centerLeftBounds.removeFromTop(32));
        mAddTrackInfo.setBounds(centerLeftBounds);
        auto centerRightBounds = centerBounds.withTrimmedLeft(2);
        mLoadTemplateButton.setBounds(centerRightBounds.removeFromTop(32));
        mLoadTemplateInfo.setBounds(centerRightBounds);
    }
}

void Application::LoaderContent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void Application::LoaderContent::applicationCommandInvoked([[maybe_unused]] juce::ApplicationCommandTarget::InvocationInfo const& info)
{
}

void Application::LoaderContent::applicationCommandListChanged()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto const& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.setTooltip(Utils::getCommandDescriptionWithKey(commandManager, CommandIDs::documentOpen));
    mAddTrackButton.setTooltip(Utils::getCommandDescriptionWithKey(commandManager, CommandIDs::editNewTrack));
    mLoadTemplateButton.setTooltip(Utils::getCommandDescriptionWithKey(commandManager, CommandIDs::editLoadTemplate));
}

void Application::DragAndDropTarget::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

bool Application::DragAndDropTarget::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto const documentHasAudioFiles = !Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>().empty();
    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    auto const importFormatsWildcard = Instance::getWildCardForImportFormats();
    auto const documentWildcard = Instance::getWildCardForDocumentFile();
    auto const fileWildcard = documentWildcard + ";" + (documentHasAudioFiles ? importFormatsWildcard : audioFormatsWildcard);
    return std::any_of(files.begin(), files.end(), [&](auto const& fileName)
                       {
                           return fileWildcard.contains(juce::File(fileName).getFileExtension());
                       });
}

void Application::DragAndDropTarget::fileDragEnter([[maybe_unused]] juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    mIsDragging = true;
    repaint();
}

void Application::DragAndDropTarget::fileDragExit([[maybe_unused]] juce::StringArray const& files)
{
    mIsDragging = false;
    repaint();
}

void Application::DragAndDropTarget::filesDropped(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    mIsDragging = false;
    repaint();

    auto const documentHasAudioFiles = !Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>().empty();
    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    auto const documentWildcard = Instance::getWildCardForDocumentFile();
    auto const openWildcard = documentWildcard + (documentHasAudioFiles ? "" : ";" + audioFormatsWildcard);
    auto const importFormatsWildcard = Instance::getWildCardForImportFormats();
    auto const getFiles = [&]()
    {
        std::vector<juce::File> openFiles;
        std::vector<juce::File> importFiles;
        for(auto const& fileName : files)
        {
            juce::File const file(fileName);
            if(openWildcard.contains(file.getFileExtension()))
            {
                openFiles.push_back(file);
            }
            else if(importFormatsWildcard.contains(file.getFileExtension()))
            {
                importFiles.push_back(file);
            }
        }
        return std::make_tuple(openFiles, importFiles);
    };
    auto const validFiles = getFiles();
    if(!std::get<0>(validFiles).empty())
    {
        Instance::get().openFiles(std::get<0>(validFiles));
    }
    else
    {
        for(auto const& importFile : std::get<1>(validFiles))
        {
            auto const position = Tools::getNewTrackPosition();
            Instance::get().importFile(position, importFile);
        }
    }
}

ANALYSE_FILE_END
