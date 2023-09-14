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
        onFileSelected(files[index]);
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

    mAdaptationInfo.setText(juce::translate("Adapt the block size and the step size of the analyzes to the sample rate"), juce::NotificationType::dontSendNotification);

    addAndMakeVisible(mSeparatorVertical);
    addAndMakeVisible(mSeparatorTop);
    addAndMakeVisible(mSelectRecentDocument);
    addAndMakeVisible(mFileTable);
    setSize(800, 600);

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
        if(Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>().empty())
        {
            addAndMakeVisible(mLoadFileButton);
            addAndMakeVisible(mLoadFileInfo);
            addAndMakeVisible(mLoadFileWildcard);
            mSelectRecentDocument.setText(juce::translate("Load a recent document"), juce::NotificationType::dontSendNotification);
        }
        else
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
                break;
            case AttrType::adaptationToSampleRate:
            {
                mAdaptationButton.setToggleState(acsr.getAttr<AttrType::adaptationToSampleRate>(), juce::NotificationType::dontSendNotification);
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
        if(mAdaptationButton.isShowing())
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

void Application::LoaderContent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void Application::LoaderContent::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

bool Application::LoaderContent::isInterestedInFileDrag(juce::StringArray const& files)
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasAudioFiles = !documentAccessor.getAttr<Document::AttrType::reader>().empty();

    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    auto const importFormatsWildcard = Instance::getWildCardForImportFormats();
    auto const documentWildcard = Instance::getWildCardForDocumentFile();
    auto const fileWildcard = documentWildcard + ";" + (documentHasAudioFiles ? importFormatsWildcard : audioFormatsWildcard);
    for(auto const& fileName : files)
    {
        if(fileWildcard.contains(juce::File(fileName).getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Application::LoaderContent::fileDragEnter([[maybe_unused]] juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    mIsDragging = true;
    repaint();
}

void Application::LoaderContent::fileDragExit([[maybe_unused]] juce::StringArray const& files)
{
    mIsDragging = false;
    repaint();
}

void Application::LoaderContent::filesDropped(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    mIsDragging = false;
    repaint();

    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasAudioFiles = !documentAccessor.getAttr<Document::AttrType::reader>().empty();

    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    auto const documentWildcard = Instance::getWildCardForDocumentFile();
    auto const openWildcard = documentWildcard + (documentHasAudioFiles ? "" : ";" + audioFormatsWildcard);
    auto const importFormatsWildcard = Instance::getWildCardForImportFormats();
    auto getFiles = [&]()
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
        return;
    }
    for(auto const& importFile : std::get<1>(validFiles))
    {
        auto const position = Tools::getNewTrackPosition();
        Instance::get().importFile(position, importFile);
    }
}

void Application::LoaderContent::applicationCommandInvoked([[maybe_unused]] juce::ApplicationCommandTarget::InvocationInfo const& info)
{
}

void Application::LoaderContent::applicationCommandListChanged()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::documentOpen));
    mAddTrackButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::editNewTrack));
    mLoadTemplateButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::editLoadTemplate));
}

ANALYSE_FILE_END
