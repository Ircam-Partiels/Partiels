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

static juce::String getAllWildCards()
{
    // clang-format off
    static juce::StringArray const wildcards
    {
          Application::Instance::getWildCardForAudioFormats()
        , Application::Instance::getWildCardForImportFormats()
        , Application::Instance::getWildCardForDocumentFile()
        , Application::Instance::getWildCardForImageFormats()
    };
    // clang-format off
    return wildcards.joinIntoString(";");
}

static bool isInterestedInFile(juce::String const& filePath)
{
    auto const file = juce::File(filePath);
    return file.existsAsFile() && getAllWildCards().contains(file.getFileExtension().toLowerCase());
}

Application::Neuralyzer::Chat::Chat(BackgroundAgent& agent)
: mAgent(agent)
, mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::stop_png, AnlIconsData::stop_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::send_png, AnlIconsData::send_pngSize))
, mAttachedFilesButton(juce::ImageCache::getFromMemory(AnlIconsData::attachment_png, AnlIconsData::attachment_pngSize))
{
    auto const updateAttachedFilesButton = [this]()
    {
        auto const files = mDroppedFilePaths.isEmpty() ? juce::translate("None") : mDroppedFilePaths.joinIntoString(", ");
        mAttachedFilesButton.setTooltip(juce::translate("Attached files: FILES").replace("FILES", files));
        mAttachedFilesButton.setToggleState(!mDroppedFilePaths.isEmpty(), juce::NotificationType::dontSendNotification);
    };

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
        auto const isInitialized = mAgent.getModelInfo().isValid();
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
        auto const isInitialized = mAgent.getModelInfo().isValid();
        auto const canSendOrStopUserQuery = isInitialized && (isRunningQuery || hasQuery);
        mSendButton.setEnabled(canSendOrStopUserQuery);
    };
    mQueryEditor.isInterestedInFile = isInterestedInFile;
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

    mAttachedFilesButton.setWantsKeyboardFocus(false);
    mAttachedFilesButton.onClick = [=, this]()
    {
        juce::PopupMenu m;
        m.addItem(juce::translate("Attach..."), [=, this]()
                  {
                      mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Choose files to attach"), juce::File{}, getAllWildCards());
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

    mHistoryEditor.onShowPopupMenu = [this](juce::PopupMenu& menu)
    {
        menu.addSeparator();
        auto const canReset = mAgent.getModelInfo().isValid();
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
                                                          if(mAgent.getModelInfo().isValid())
                                                          {
                                                              clearHistory();
                                                          }
                                                      });
                     });
        menu.addItem(juce::translate("Open History"), getTempHistoryFile().existsAsFile(), false, []()
                     {
                         getTempHistoryFile().startAsProcess();
                     });
    };
    mSendButton.onClick = [this]()
    {
        if(!isConfigured())
        {
            askToConfigureNeuralyzer();
        }
        else if(mAgent.getCurrentAction() == BackgroundAgent::Action::sendQuery)
        {
            stopUserQuery();
        }
        else if(mAgent.getModelInfo().isValid())
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

    addAndMakeVisible(mHistoryEditor);
    addAndMakeVisible(mSeparator3);
    addChildComponent(mTempResponse);
    addChildComponent(mSeparator2);
    addAndMakeVisible(mQueryEditor);
    addAndMakeVisible(mSendButton);
    addAndMakeVisible(mAttachedFilesButton);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);
    addAndMakeVisible(mUsageLabel);

    mAgent.addChangeListener(this);
    updated();
}

Application::Neuralyzer::Chat::~Chat()
{
    mAgent.removeChangeListener(this);
    mQueryEditor.removeMouseListener(this);
}

bool Application::Neuralyzer::Chat::isConfigured()
{
    auto const& accessor = mAgent.getAccessor();
    return accessor.getAttr<AttrType::agentBackend>() != AgentBackend::none && accessor.getAttr<AttrType::modelInfo>().isValid();
}

void Application::Neuralyzer::Chat::mouseDown(juce::MouseEvent const& event)
{
    if(event.eventComponent == &mQueryEditor)
    {
        if(!isConfigured())
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
    mSendButton.setBounds(menuArea.removeFromRight(18));
    mAttachedFilesButton.setBounds(menuArea.removeFromLeft(18));
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
    updateHistory();
}

void Application::Neuralyzer::Chat::parentHierarchyChanged()
{
    updateHistory();
}

void Application::Neuralyzer::Chat::sendUserQuery()
{
    auto const query = mQueryEditor.getText().trim();
    MiscWeakAssert(query.isNotEmpty());
    if(mAgent.getCurrentAction() != BackgroundAgent::Action::none || query.isEmpty())
    {
        return;
    }

    mQueryEditor.setText({}, true);
    mQueryEditor.resetHistoryIndex();
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));
    mAgent.sendQuery(query, mDroppedFilePaths);
    mTempResponse.setText(juce::translate("Processing query"), false);
    mTempResponse.setVisible(true);
    mSeparator2.setVisible(true);
    mQueryEditor.onFilesDropped({});
    startTimer(250);
    updated();
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
    updated();
    resized();
}

void Application::Neuralyzer::Chat::clearHistory()
{
    MiscWeakAssert(mAgent.getModelInfo().isValid());
    if(!mAgent.getModelInfo().isValid())
    {
        return;
    }

    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);
    mAgent.startSession();
    startTimer(250);
    updated();
}

void Application::Neuralyzer::Chat::updateHistory()
{
    using MessageType = Agent::MessageType;
    getTempHistoryFile().deleteFile();
    mHistory.clear();
    mTables.clear();
    if(!mAgent.getModelInfo().isValid())
    {
        auto const lastResult = mAgent.getLastResult();
        if(lastResult.failed())
        {
            mHistory.push_back({MessageType::warning, lastResult.getErrorMessage()});
        }
        return;
    }

    auto history = mAgent.getHistory();
    auto fullHistoryStream = getTempHistoryFile().createOutputStream();
    mHistory.emplace_back(MessageType::assistant, juce::translate("I'm Neuralyzer, your AI assistant. How can I help you today?"));
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
        else if(message.first == MessageType::warning)
        {
            mHistory.push_back(message);
        }
    }
    auto const lastResult = mAgent.getLastResult();
    if(lastResult.failed())
    {
        mHistory.push_back({MessageType::warning, lastResult.getErrorMessage()});
    }
    mHistoryEditor.setHistory(mHistory);
    mQueryEditor.setHistory(mHistory);
}

void Application::Neuralyzer::Chat::updated()
{
    timerCallback();

    auto const modelInfo = mAgent.getModelInfo();
    auto const currentAction = mAgent.getCurrentAction();
    if(!isConfigured())
    {
        mStatusLabel.setText(juce::translate("Configure Neuralyzer in the settings panel."), juce::dontSendNotification);
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Neuralyzer is not configured"), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    else if(currentAction == BackgroundAgent::Action::none)
    {
        if(!mAgent.hasPendingAction())
        {
            stopTimer();
        }
        auto const result = mAgent.getLastResult();
        if(result.failed())
        {
            mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", result.getErrorMessage()), juce::dontSendNotification);
            mQueryEditor.setTextToShowWhenEmpty(juce::translate("Error: ERROR").replace("ERROR", result.getErrorMessage()), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
        }
        else if(modelInfo.isValid())
        {
            mStatusLabel.setText(juce::translate("Enter your query in natural language..."), juce::dontSendNotification);
            mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
        }
        mTempResponse.setVisible(false);
        mSeparator2.setVisible(false);
        mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        mSendButton.setTooltip(juce::translate("Send Query"));
        mLastDocumentAccessorState.copyFrom(Application::Instance::get().getDocumentAccessor(), NotificationType::synchronous);
    }
    else
    {
        if(!isTimerRunning())
        {
            startTimer(250);
        }
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }

    if(modelInfo.isValid())
    {
        mUsageLabel.setText(juce::String(static_cast<int>(mAgent.getContextCapacityUsage() * 100.0f)) + "%", juce::dontSendNotification);
    }
    else
    {
        mUsageLabel.setText({}, juce::dontSendNotification);
    }

    mQueryEditor.onTextChange();
    updateHistory();
    colourChanged();
    resized();
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
    updated();
}

ANALYSE_FILE_END
