#include "AnlApplicationNeuralyzerChat.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

static void askToConfigureNeuralyzer()
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("Neuralyzer is not configured"))
                             .withMessage(juce::translate("The Neuralyzer is not configured yet. Would you like to open the settings panel to configure it now?"))
                             .withButton(juce::translate("Open"))
                             .withButton(juce::translate("Cancel"));
    juce::AlertWindow::showAsync(options, [](int windowResult)
                                 {
                                     if(windowResult != 1)
                                     {
                                         return;
                                     }
                                     if(auto* window = Application::Instance::get().getWindow())
                                     {
                                         window->getInterface().showNeuralyzerSettingsPanel();
                                     }
                                 });
}

static juce::String getAllWildCard()
{
    auto const audioFormatsWildcard = Application::Instance::getWildCardForAudioFormats();
    auto const importFormatsWildcard = Application::Instance::getWildCardForImportFormats();
    auto const documentWildcard = Application::Instance::getWildCardForDocumentFile();
    return documentWildcard + ";" + importFormatsWildcard + ";" + audioFormatsWildcard;
}

static bool isInterestedInFile(juce::String const& fileName)
{
    auto const file = juce::File(fileName);
    return file.existsAsFile() && getAllWildCard().contains(file.getFileExtension().toLowerCase());
}

juce::File Application::Neuralyzer::Chat::mFullHistoryFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)
                                                                 .getChildFile("neuralyzer-history.md");

Application::Neuralyzer::Chat::Chat(Accessor& accessor, BackgroundAgent& agent, Rag::Engine& ragEngine)
: mAccessor(accessor)
, mAgent(agent)
, mRagEngine(ragEngine)
, mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::stop_png, AnlIconsData::stop_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::send_png, AnlIconsData::send_pngSize))
, mSelectionStateButton(juce::ImageCache::getFromMemory(AnlIconsData::selection_png, AnlIconsData::selection_pngSize))
, mAttachedFilesButton(juce::ImageCache::getFromMemory(AnlIconsData::attachment_png, AnlIconsData::attachment_pngSize))
, mSelectionLayoutNotifier(typeid(*this).name(), Application::Instance::get().getDocumentAccessor(), nullptr, {Group::AttrType::focused}, {Track::AttrType::focused})
{

    auto const updateSelectionStateButton = [this]()
    {
        auto& documentAcsr = Application::Instance::get().getDocumentAccessor();
        auto const selectedGroups = Document::Selection::getGroups(documentAcsr);
        auto const selectedTracks = Document::Selection::getTracks(documentAcsr);
        auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
        auto const timeSelection = transportAcsr.getAttr<Transport::AttrType::selection>();

        juce::StringArray parts;
        parts.add(juce::translate("Include selected tracks and groups in the query"));
        if(!selectedGroups.empty())
        {
            juce::StringArray groupNames;
            for(auto const& id : selectedGroups)
            {
                if(Document::Tools::hasGroupAcsr(documentAcsr, id))
                {
                    groupNames.add(Document::Tools::getGroupAcsr(documentAcsr, id).getAttr<Group::AttrType::name>());
                }
            }
            parts.add(juce::translate("Selected groups: NAMES").replace("NAMES", groupNames.joinIntoString(", ")));
        }
        if(!selectedTracks.empty())
        {
            juce::StringArray trackNames;
            for(auto const& id : selectedTracks)
            {
                if(Document::Tools::hasTrackAcsr(documentAcsr, id))
                {
                    trackNames.add(Document::Tools::getTrackAcsr(documentAcsr, id).getAttr<Track::AttrType::name>());
                }
            }
            parts.add(juce::translate("Selected tracks: NAMES").replace("NAMES", trackNames.joinIntoString(", ")));
        }
        if(!timeSelection.isEmpty())
        {
            parts.add(juce::translate("Time selection: START - END")
                          .replace("START", juce::String(timeSelection.getStart(), 3))
                          .replace("END", juce::String(timeSelection.getEnd(), 3)));
        }
        mSelectionStateButton.setTooltip(parts.joinIntoString("\n"));
        mSelectionStateButton.setToggleState(!selectedGroups.empty() || !selectedTracks.empty() || !timeSelection.isEmpty(), juce::NotificationType::dontSendNotification);
    };

    auto const updateAttachedFilesButton = [this]()
    {
        auto const files = mDroppedFilePaths.isEmpty() ? juce::translate("None") : mDroppedFilePaths.joinIntoString(", ");
        mAttachedFilesButton.setTooltip(juce::translate("Attached files: FILES").replace("FILES", files));
        mAttachedFilesButton.setToggleState(!mDroppedFilePaths.isEmpty(), juce::NotificationType::dontSendNotification);
    };

    mHistoryEditor.setReadOnly(true);
    mHistoryEditor.setMultiLine(true);
    mHistoryEditor.setScrollbarsShown(true);
    mHistoryEditor.setCaretVisible(true);
    mHistoryEditor.setPopupMenuEnabled(false);

    mQueryEditor.setMultiLine(true);
    mQueryEditor.setReturnKeyStartsNewLine(false);
    mQueryEditor.setScrollbarsShown(true);
    mQueryEditor.setCaretVisible(true);
    mQueryEditor.setPopupMenuEnabled(true);
    mQueryEditor.setFont(juce::Font(juce::FontOptions(14.0f)));
    mQueryEditor.onReturnKey = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        auto const isRunningQuery = mAgent.getCurrentAction() != BackgroundAgent::Action::none;
        auto const isInitialized = mAgent.getModelInfo().valid();
        auto const canSendUserQuery = isInitialized && hasQuery && !isRunningQuery;
        if(canSendUserQuery)
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        auto const isRunningQuery = mAgent.getCurrentAction() == BackgroundAgent::Action::sendQuery;
        auto const isInitialized = mAgent.getModelInfo().valid();
        auto const canSendOrStopUserQuery = isInitialized && (isRunningQuery || hasQuery);
        mSendButton.setEnabled(canSendOrStopUserQuery);
    };
    mQueryEditor.onFilesDragEnter = [this](juce::StringArray const& files)
    {
        mIsDragging = std::any_of(files.begin(), files.end(), isInterestedInFile);
        repaint();
    };
    mQueryEditor.onFilesDropped = [=, this](juce::StringArray const& files)
    {
        for(auto const& file : files)
        {
            if(isInterestedInFile(file) && !mDroppedFilePaths.contains(file))
            {
                mDroppedFilePaths.add(file);
            }
        }
        updateAttachedFilesButton();
        mIsDragging = false;
        repaint();
    };
    mQueryEditor.onFilesDragExit = [this]()
    {
        mIsDragging = false;
        repaint();
        resized();
    };
    mQueryEditor.addMouseListener(this, true);

    mSelectionStateButton.setWantsKeyboardFocus(false);
    mSelectionStateButton.onClick = [this]()
    {
        auto& documentAcsr = Application::Instance::get().getDocumentAccessor();
        auto const selectedItems = Document::Selection::getItems(documentAcsr);
        auto const& transportAcsr = documentAcsr.getAcsr<Document::AcsrType::transport>();
        auto const timeSelection = transportAcsr.getAttr<Transport::AttrType::selection>();

        juce::PopupMenu m;
        m.addItem(juce::translate("Reset time selection"), !timeSelection.isEmpty(), false, [=]()
                  {
                      auto& docAcsr = Application::Instance::get().getDocumentAccessor();
                      docAcsr.getAcsr<Document::AcsrType::transport>().setAttr<Transport::AttrType::selection>(juce::Range<double>{}, NotificationType::synchronous);
                  });
        m.addSeparator();
        m.addItem(juce::translate("Deselect all"), !std::get<0>(selectedItems).empty() || !std::get<1>(selectedItems).empty(), false, []()
                  {
                      auto& docAcsr = Application::Instance::get().getDocumentAccessor();
                      Document::Selection::clearAll(docAcsr, NotificationType::synchronous);
                      docAcsr.getAcsr<Document::AcsrType::transport>().setAttr<Transport::AttrType::selection>(juce::Range<double>{}, NotificationType::synchronous);
                  });
        m.addSeparator();
        for(auto const& groupId : std::get<0>(selectedItems))
        {
            auto const name = Document::Tools::getGroupAcsr(documentAcsr, groupId).getAttr<Group::AttrType::name>();
            m.addItem(juce::translate("Deselect group NAME").replace("NAME", name), [=]()
                      {
                          auto& docAcsr = Application::Instance::get().getDocumentAccessor();
                          if(Document::Tools::hasGroupAcsr(docAcsr, groupId))
                          {
                              Document::Tools::getGroupAcsr(docAcsr, groupId).setAttr<Group::AttrType::focused>(Group::FocusInfo{}, NotificationType::synchronous);
                          }
                      });
        }
        for(auto const& trackId : std::get<1>(selectedItems))
        {
            auto const name = Document::Tools::getTrackAcsr(documentAcsr, trackId).getAttr<Track::AttrType::name>();
            m.addItem(juce::translate("Deselect track NAME").replace("NAME", name), [=]()
                      {
                          auto& docAcsr = Application::Instance::get().getDocumentAccessor();
                          if(Document::Tools::hasTrackAcsr(docAcsr, trackId))
                          {
                              Document::Tools::getTrackAcsr(docAcsr, trackId).setAttr<Track::AttrType::focused>(Track::FocusInfo{}, NotificationType::synchronous);
                          }
                      });
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mSelectionStateButton).withDeletionCheck(*this));
    };

    mAttachedFilesButton.setWantsKeyboardFocus(false);
    mAttachedFilesButton.onClick = [=, this]()
    {
        juce::PopupMenu m;
        m.addItem(juce::translate("Attach..."), [=, this]()
                  {
                      mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Choose files to attach"), juce::File{}, getAllWildCard());
                      auto const flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems;
                      auto const weakReference = juce::WeakReference<juce::Component>(this);
                      mFileChooser->launchAsync(flags, [=, this](juce::FileChooser const& fc)
                                                {
                                                    if(weakReference == nullptr)
                                                    {
                                                        return;
                                                    }
                                                    for(auto const& file : fc.getResults())
                                                    {
                                                        auto const filePath = file.getFullPathName();
                                                        if(isInterestedInFile(filePath) && !mDroppedFilePaths.contains(filePath))
                                                        {
                                                            mDroppedFilePaths.add(filePath);
                                                        }
                                                    }
                                                    updateAttachedFilesButton();
                                                });
                  });
        if(mDroppedFilePaths.size() > 1)
        {
            m.addItem(juce::translate("Detach all"), [=, this]()
                      {
                          mDroppedFilePaths.clearQuick();
                          updateAttachedFilesButton();
                      });
            m.addSeparator();
        }
        for(auto const& file : mDroppedFilePaths)
        {
            m.addItem(juce::translate("Detach NAME").replace("NAME", juce::File(file).getFileName()), [=, this]()
                      {
                          mDroppedFilePaths.removeString(file);
                          updateAttachedFilesButton();
                      });
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mAttachedFilesButton).withDeletionCheck(*this));
    };

    mMenuButton.setEdgeIndent(1);
    mMenuButton.onClick = [this]()
    {
        juce::PopupMenu menu;
        auto const canReset = mAgent.getModelInfo().valid();
        menu.addItem(juce::translate("Reset History"), canReset, false, [this]()
                     {
                         auto const options = juce::MessageBoxOptions()
                                                  .withIconType(juce::AlertWindow::QuestionIcon)
                                                  .withTitle(juce::translate("Reset the Neuralyzer history?"))
                                                  .withMessage(juce::translate("Are you sure you want to reset the Neuralyzer history? This will clear the current chat history and temporary response."))
                                                  .withButton(juce::translate("Ok"))
                                                  .withButton(juce::translate("Cancel"))
                                                  .withAssociatedComponent(this);
                         juce::AlertWindow::showAsync(options, [this](int result)
                                                      {
                                                          if(result != 1)
                                                          {
                                                              return;
                                                          }
                                                          if(mAgent.getModelInfo().valid())
                                                          {
                                                              clearHistory();
                                                          }
                                                      });
                     });
        menu.addItem(juce::translate("Open History"), mFullHistoryFile.existsAsFile(), false, []()
                     {
                         mFullHistoryFile.startAsProcess();
                     });
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mMenuButton).withDeletionCheck(*this));
    };
    mSendButton.onClick = [this]()
    {
        if(!mAgent.getModelInfo().valid() && mAgent.getCurrentAction() != BackgroundAgent::Action::initializeModel)
        {
            askToConfigureNeuralyzer();
        }
        else if(mAgent.getCurrentAction() == BackgroundAgent::Action::sendQuery)
        {
            stopUserQuery();
        }
        else
        {
            sendUserQuery();
        }
    };
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);

    mStatusLabel.setText(juce::translate("Model not initialized"), juce::dontSendNotification);
    mStatusLabel.setJustificationType(juce::Justification::centredLeft);
    mStatusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    mStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    mUsageLabel.setText({}, juce::dontSendNotification);
    mUsageLabel.setJustificationType(juce::Justification::centredRight);
    mUsageLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    mUsageLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    mTempResponse.setReadOnly(true);
    mTempResponse.setMultiLine(true);
    mTempResponse.setScrollbarsShown(false);
    mTempResponse.setCaretVisible(false);
    mTempResponse.setPopupMenuEnabled(false);

    mSelectionLayoutNotifier.onLayoutUpdated = [=]()
    {
        updateSelectionStateButton();
    };
    mTransportListener.onAttrChanged = [=]([[maybe_unused]] Transport::Accessor const& acsr, Transport::AttrType attr)
    {
        switch(attr)
        {
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
                break;
            case Transport::AttrType::selection:
            {
                updateSelectionStateButton();
                break;
            }
        }
    };
    Application::Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().addListener(mTransportListener, NotificationType::synchronous);

    addAndMakeVisible(mHistoryEditor);
    addAndMakeVisible(mSeparator3);
    addChildComponent(mTempResponse);
    addChildComponent(mSeparator2);
    addAndMakeVisible(mQueryEditor);
    addAndMakeVisible(mSendButton);
    addAndMakeVisible(mSelectionStateButton);
    addAndMakeVisible(mAttachedFilesButton);
    addAndMakeVisible(mMenuButton);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);
    addAndMakeVisible(mUsageLabel);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelInfo:
            {
                if(acsr.getAttr<AttrType::modelInfo>().valid())
                {
                    initializeSystem();
                }
                break;
            }
            case AttrType::effectiveState:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAgent.addChangeListener(this);
    updateHistory();
}

Application::Neuralyzer::Chat::~Chat()
{
    mAgent.removeChangeListener(this);
    Application::Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>().removeListener(mTransportListener);
    mQueryEditor.removeMouseListener(this);
    mAccessor.removeListener(mListener);
}

void Application::Neuralyzer::Chat::mouseDown(juce::MouseEvent const& event)
{
    if(event.eventComponent == &mQueryEditor)
    {
        if(!mAgent.getModelInfo().valid() && !mAccessor.getAttr<AttrType::modelInfo>().valid())
        {
            askToConfigureNeuralyzer();
        }
    }
}

void Application::Neuralyzer::Chat::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::Neuralyzer::Chat::paintOverChildren(juce::Graphics& g)
{
    if(mIsDragging)
    {
        g.setColour(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
        g.fillRect(mQueryEditor.getBounds().expanded(4, 0));
    }
}

void Application::Neuralyzer::Chat::resized()
{
    auto bounds = getLocalBounds();
    auto statusBounds = bounds.removeFromBottom(20);
    mUsageLabel.setBounds(statusBounds.removeFromRight(40));
    mStatusLabel.setBounds(statusBounds);
    mSeparator3.setBounds(bounds.removeFromBottom(1));
    auto menuArea = bounds.removeFromBottom(22).reduced(4, 2);
    auto bottomArea = menuArea;
    mMenuButton.setBounds(menuArea.removeFromBottom(8).removeFromRight(8));
    bottomArea.removeFromRight(8);
    mSendButton.setBounds(bottomArea.removeFromRight(18));
    mSelectionStateButton.setBounds(bottomArea.removeFromLeft(18));
    bottomArea.removeFromLeft(2);
    mAttachedFilesButton.setBounds(bottomArea.removeFromLeft(18));
    mQueryEditor.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 7).reduced(4, 0));
    if(mTempResponse.isVisible())
    {
        mSeparator2.setBounds(bounds.removeFromBottom(1));
        mTempResponse.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 3).reduced(4, 0));
    }
    mSeparator1.setBounds(bounds.removeFromBottom(1));
    mHistoryEditor.setBounds(bounds.reduced(4, 0));
}

void Application::Neuralyzer::Chat::colourChanged()
{
    auto const normalColour = findColour(juce::TextEditor::ColourIds::textColourId).withMultipliedAlpha(0.75f);
    auto const overColour = normalColour.brighter(0.25f);
    auto const downColour = normalColour.darker(0.25f);

    juce::Path triangle;
    triangle.addTriangle({0.0f, 0.0f}, {10.0f, 0.0f}, {5.0f, 10.0f});

    juce::DrawablePath normalPath;
    normalPath.setPath(triangle);
    normalPath.setFill(normalColour);

    juce::DrawablePath overPath;
    overPath.setPath(triangle);
    overPath.setFill(overColour);

    juce::DrawablePath downPath;
    downPath.setPath(triangle);
    downPath.setFill(downColour);
    mMenuButton.setImages(&normalPath, &overPath, &downPath);

    updateHistory();
}

void Application::Neuralyzer::Chat::parentHierarchyChanged()
{
    colourChanged();
}

void Application::Neuralyzer::Chat::handleAsyncUpdate()
{
    timerCallback();

    auto const modelInfo = mAgent.getModelInfo();
    auto const currentAction = mAgent.getCurrentAction();
    if(currentAction == BackgroundAgent::Action::none)
    {
        if(!mAgent.hasPendingAction())
        {
            stopTimer();
        }
        auto const result = mAgent.getLastResult();
        if(result.failed())
        {
            mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", result.getErrorMessage()), juce::dontSendNotification);
        }
        else if(modelInfo.valid())
        {
            mStatusLabel.setText(juce::translate("Enter your query in natural language..."), juce::dontSendNotification);
        }
        else
        {
            mStatusLabel.setText(juce::translate("Model not initialized"), juce::dontSendNotification);
        }
        mTempResponse.setVisible(false);
        mSeparator2.setVisible(false);
        mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        mSendButton.setTooltip(juce::translate("Send Query"));
        mLastDocumentAccessorState.copyFrom(Application::Instance::get().getDocumentAccessor(), NotificationType::synchronous);
    }
    else if(!isTimerRunning())
    {
        startTimer(250);
    }

    if(!modelInfo.valid() && mAgent.getCurrentAction() == BackgroundAgent::Action::none)
    {
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Select a model in the settings panel."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    else if(modelInfo.valid())
    {
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }

    mAccessor.setAttr<AttrType::effectiveState>(modelInfo, NotificationType::synchronous);
    if(modelInfo.valid())
    {
        mUsageLabel.setText(juce::String(static_cast<int>(mAgent.getContextCapacityUsage() * 100.0f)) + "%", juce::dontSendNotification);
        juce::StringArray modelInfoParts;
        modelInfoParts.add(juce::translate("Model: MODEL").replace("MODEL", modelInfo.modelFile.getFileNameWithoutExtension()));
        if(modelInfo.contextSize.has_value())
        {
            modelInfoParts.add(juce::translate("Context Size: SIZE").replace("SIZE", juce::String(modelInfo.contextSize.value())));
        }
        if(modelInfo.batchSize.has_value())
        {
            modelInfoParts.add(juce::translate("Batch Size: SIZE").replace("SIZE", juce::String(modelInfo.batchSize.value())));
        }
        if(modelInfo.minP.has_value())
        {
            modelInfoParts.add(juce::translate("Min P: VALUE").replace("VALUE", juce::String(modelInfo.minP.value(), 2)));
        }
        if(modelInfo.temperature.has_value())
        {
            modelInfoParts.add(juce::translate("Temperature: VALUE").replace("VALUE", juce::String(modelInfo.temperature.value(), 2)));
        }
        mUsageLabel.setTooltip(modelInfoParts.joinIntoString(" / "));
        mStatusLabel.setTooltip(modelInfoParts.joinIntoString(" / "));
    }
    else
    {
        mUsageLabel.setText({}, juce::dontSendNotification);
        mUsageLabel.setTooltip({});
        mStatusLabel.setTooltip({});
    }

    mQueryEditor.onTextChange();
    updateHistory();
    colourChanged();
    resized();
}

void Application::Neuralyzer::Chat::initializeSystem()
{
    auto info = mAccessor.getAttr<AttrType::modelInfo>();
    MiscWeakAssert(info.valid());
    if(!info.valid())
    {
        return;
    }

    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    resized();

    // Force enable to force the button to update its look,
    // then disable it again until the user enters a query
    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);

    updateHistory();

    mAgent.initializeModel(info);
    startTimer(250);
    handleAsyncUpdate();
}

void Application::Neuralyzer::Chat::sendUserQuery()
{
    auto const query = mQueryEditor.getText().trim();
    MiscWeakAssert(query.isNotEmpty());
    if(mAgent.getCurrentAction() != BackgroundAgent::Action::none || query.isEmpty())
    {
        return;
    }

    juce::String queryForModel;
    // Adds the RAG context to the query
    // With normalized embeddings + inner-product space, lower distance means better match.
    static constexpr float ragMaxDistance = 0.45f;
    auto const contexts = mRagEngine.computeContext(query, ragMaxDistance);
    if(!contexts.empty())
    {
        queryForModel << "Relevant documentation excerpts:\n";
        for(auto const& context : contexts)
        {
            queryForModel << "\n";
            queryForModel << "Source: " << context.document << "\n";
            queryForModel << "Section: " << context.section << "\n";
            // Remove all line breaks, tabs, etc. from the excerpt to avoid formatting issues in the model's response
            queryForModel << "Excerpt: " << context.excerpt.replaceCharacters("\n\r\t", "   ") << "\n";
        }
        queryForModel << "\n";
    }

    // Adds the selected items to the query
    auto& documentAcsr = Application::Instance::get().getDocumentAccessor();
    auto const selectedItems = Document::Selection::getItems(documentAcsr);
    if(!std::get<0>(selectedItems).empty() || !std::get<1>(selectedItems).empty())
    {
        queryForModel << "Item selected in the document (identifiers):\n";
        for(auto const& id : std::get<0>(selectedItems))
        {
            queryForModel << "- Group: " << id << "\n";
        }
        for(auto const& id : std::get<1>(selectedItems))
        {
            queryForModel << "- Track: " << id << "\n";
        }
        queryForModel << "\n";
    }

    // Adds the time selection to the query
    auto const timeSelection = documentAcsr.getAcsr<Document::AcsrType::transport>().getAttr<Transport::AttrType::selection>();
    if(!timeSelection.isEmpty())
    {
        queryForModel << "Time selection:\n";
        queryForModel << "- Start: " << juce::String(timeSelection.getStart(), 3) << "s\n";
        queryForModel << "- End: " << juce::String(timeSelection.getEnd(), 3) << "s\n";
        queryForModel << "\n";
    }

    // Adds a note to the query if the document has changed
    if(!mLastDocumentAccessorState.isEquivalentTo(documentAcsr))
    {
        queryForModel << "Note: The document may have changed since the last request.\n";
        queryForModel << "\n";
    }

    // Adds the attached files to the query
    if(!mDroppedFilePaths.isEmpty())
    {
        queryForModel << "Attached file paths:\n";
        for(auto const& fileName : mDroppedFilePaths)
        {
            queryForModel << "- " << fileName << "\n";
        }
        queryForModel << "\n";
    }

    // Adds the user request to the query
    queryForModel << "User request:\n" + query;

    mQueryEditor.setText({}, true);
    mQueryEditor.resetHistoryIndex();
    mDroppedFilePaths.clear();
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mAgent.sendQuery(queryForModel);
    mTempResponse.setText(juce::translate("Processing query"), false);
    mTempResponse.setVisible(true);
    mSeparator2.setVisible(true);
    startTimer(250);
    handleAsyncUpdate();
    updateHistory();
    resized();
}

void Application::Neuralyzer::Chat::stopUserQuery()
{
    MiscWeakAssert(mAgent.getCurrentAction() == BackgroundAgent::Action::sendQuery);
    if(mAgent.getCurrentAction() != BackgroundAgent::Action::sendQuery)
    {
        return;
    }

    mAgent.cancelQuery();
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    startTimer(250);
    handleAsyncUpdate();
    resized();
}

void Application::Neuralyzer::Chat::clearHistory()
{
    MiscWeakAssert(mAgent.getModelInfo().valid());
    if(!mAgent.getModelInfo().valid())
    {
        return;
    }

    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);
    updateHistory();
    mAgent.startSession();
    startTimer(250);
    handleAsyncUpdate();
}

void Application::Neuralyzer::Chat::updateHistory()
{
    auto const userName = juce::String(juce::CharPointer_UTF8("\xf0\x9f\x91\xa4 ")) + juce::translate("User"); // 👤
    auto const userColour = findColour(juce::TextEditor::ColourIds::textColourId);
    auto const assistantName = juce::String(juce::CharPointer_UTF8("\xf0\x9f\xa4\x96 ")) + juce::translate("Assistant"); // 🤖
    auto const assistantColour = juce::Colours::green;
    auto const font = juce::Font(juce::FontOptions(14.0f));

    auto history = mAgent.getHistory();
    mFullHistoryFile.deleteFile();
    auto fullHistoryStream = mFullHistoryFile.createOutputStream();
    mHistory.clear();
    for(auto const& message : history)
    {
        if(fullHistoryStream != nullptr)
        {
            static auto const lineBreak = "  \n";
            *fullHistoryStream << juce::String(std::string(magic_enum::enum_name(message.first))) << ":" << lineBreak;
            *fullHistoryStream << message.second << lineBreak << lineBreak;
            *fullHistoryStream << "---" << lineBreak;
        }

        if(message.first == MessageType::assistant)
        {
            if(mHistory.empty() || mHistory.back().first != MessageType::assistant)
            {
                mHistory.push_back(message);
            }
            else
            {
                mHistory.back().second = message.second;
            }
        }
        else if(message.first == MessageType::user && !mHistory.empty())
        {
            auto text = message.second.fromLastOccurrenceOf("User request:\n", false, false).upToLastOccurrenceOf("\n\n---", false, false);
            mHistory.push_back({MessageType::user, std::move(text)});
        }
    }
    auto const addText = [&](auto const& role, auto const& request, auto const& colour)
    {
        mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, colour);
        mHistoryEditor.setFont(font.boldened());
        mHistoryEditor.insertTextAtCaret(role + ":\n");
        mHistoryEditor.setFont(font);
        Formatter::addMarkdown(mHistoryEditor, request);
        mHistoryEditor.insertTextAtCaret("\n");
    };

    mHistoryEditor.clear();
    for(auto const& [role, request] : mHistory)
    {
        switch(role)
        {
            case MessageType::user:
                addText(userName, request, userColour);
                break;
            case MessageType::assistant:
                addText(assistantName, request, assistantColour);
                break;
            case MessageType::system:
            case MessageType::tool:
                break;
        }
    }
    mHistoryEditor.scrollDown();
}

void Application::Neuralyzer::Chat::timerCallback()
{
    auto const currentAction = mAgent.getCurrentAction();
    auto newText = [&]() -> juce::String
    {
        switch(currentAction)
        {
            case BackgroundAgent::Action::setupSystem:
            {
                return juce::translate("Setup system");
            }
            case BackgroundAgent::Action::initializeModel:
            {
                return juce::translate("Initializing model");
            }
            case BackgroundAgent::Action::startSession:
            {
                return juce::translate("Starting session");
            }
            case BackgroundAgent::Action::loadSession:
            {
                return juce::translate("Loading session");
            }
            case BackgroundAgent::Action::sendQuery:
            {
                return juce::translate("Processing query");
            }
            case BackgroundAgent::Action::saveSession:
            {
                return juce::translate("Saving session");
            }
            case BackgroundAgent::Action::cancelQuery:
            {
                return juce::translate("Stopping query");
            }
            case BackgroundAgent::Action::none:
            {
                return {};
            }
        }
        return {};
    }();
    if(currentAction != BackgroundAgent::Action::none)
    {
        auto const text = mStatusLabel.getText();
        // Find the number of dots at the end of the text
        auto const index = text.indexOf(".");
        auto const numDots = (index >= 0) ? text.length() - index : 0;
        for(auto i = 0; i < (numDots + 1) % 4; ++i)
        {
            newText += ".";
        }
    }
    mStatusLabel.setText(newText, juce::dontSendNotification);
    auto const responseText = mAgent.getTemporaryResponse();
    if(responseText.isEmpty())
    {
        mTempResponse.setText(newText, false);
    }
    else
    {
        mTempResponse.setText(responseText, false);
        mTempResponse.moveCaretToEnd();
    }
    mUsageLabel.setText(juce::String(static_cast<int>(mAgent.getContextCapacityUsage() * 100.0f)) + "%", juce::dontSendNotification);
}

void Application::Neuralyzer::Chat::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    handleAsyncUpdate();
}

Application::Neuralyzer::Chat::QueryEditor::QueryEditor(History const& history)
: mHistory(history)
{
    addListener(this);
}

bool Application::Neuralyzer::Chat::QueryEditor::keyPressed(juce::KeyPress const& key)
{
    auto const numUserMessages = std::count_if(mHistory.cbegin(), mHistory.cend(), [](auto const& message)
                                               {
                                                   return std::get<0_z>(message) == MessageType::user;
                                               });
    if(key.isKeyCode(juce::KeyPress::upKey) && getCaretPosition() == 0 && mHistoryIndex < numUserMessages)
    {
        int index = 0;
        for(auto historyIt = mHistory.crbegin(); historyIt != mHistory.crend(); ++historyIt)
        {
            if(std::get<0_z>(*historyIt) == MessageType::user)
            {
                if(index == mHistoryIndex + 1)
                {
                    ++mHistoryIndex;
                    mShouldChange = false;
                    setText(std::get<1_z>(*historyIt), true);
                    return true;
                }
                ++index;
            }
        }
    }
    else if(key.isKeyCode(juce::KeyPress::downKey) && getCaretPosition() == getTotalNumChars() && mHistoryIndex > -1)
    {
        if(mHistoryIndex > 0)
        {
            int index = 0;
            for(auto historyIt = mHistory.crbegin(); historyIt != mHistory.crend(); ++historyIt)
            {
                if(std::get<0_z>(*historyIt) == MessageType::user)
                {
                    if(index == mHistoryIndex - 1)
                    {
                        --mHistoryIndex;
                        mShouldChange = false;
                        setText(std::get<1_z>(*historyIt), true);
                        return true;
                    }
                    ++index;
                }
            }
        }
        else
        {
            mHistoryIndex = -1;
            mShouldChange = false;
            setText(mSavedText, true);
            return true;
        }
    }
    return juce::TextEditor::keyPressed(key);
}

void Application::Neuralyzer::Chat::QueryEditor::resetHistoryIndex()
{
    mHistoryIndex = -1;
}

void Application::Neuralyzer::Chat::QueryEditor::textEditorTextChanged(juce::TextEditor&)
{
    if(std::exchange(mShouldChange, true))
    {
        mSavedText = getText();
    }
}

bool Application::Neuralyzer::Chat::QueryEditor::isInterestedInFileDrag(juce::StringArray const& files)
{
    return std::any_of(files.begin(), files.end(), isInterestedInFile);
}

void Application::Neuralyzer::Chat::QueryEditor::fileDragEnter(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    if(onFilesDragEnter != nullptr)
    {
        onFilesDragEnter(files);
    }
}

void Application::Neuralyzer::Chat::QueryEditor::fileDragExit([[maybe_unused]] juce::StringArray const& files)
{
    if(onFilesDragExit != nullptr)
    {
        onFilesDragExit();
    }
}

void Application::Neuralyzer::Chat::QueryEditor::filesDropped(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    if(onFilesDropped != nullptr)
    {
        onFilesDropped(files);
    }
}

ANALYSE_FILE_END
