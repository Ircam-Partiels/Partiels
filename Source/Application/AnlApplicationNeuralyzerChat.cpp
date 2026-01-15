#include "AnlApplicationNeuralyzerChat.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

static void askToConfigureNeuralyzer()
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("The agent is not configured"))
                             .withMessage(juce::translate("The agent is not configured yet. Would you like to open the settings panel to configure it now?"))
                             .withButton(juce::translate("Open"), 1)
                             .withButton(juce::translate("Cancel"), 0);
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

static void updateUsageLabel(juce::Label& label, Application::Neuralyzer::BackgroundAgent const& agent)
{
    auto const modelInfo = agent.getModelInfo();
    if(!modelInfo.isValid())
    {
        label.setText({}, juce::dontSendNotification);
        label.setTooltip({});
        return;
    }
    auto const usage = agent.getContextCapacityUsage();
    label.setText(juce::String(static_cast<int>(usage * 100.0f)) + "%", juce::dontSendNotification);
    if(modelInfo.contextSize.has_value())
    {
        auto const contextSize = modelInfo.contextSize.value();
        auto const usedTokens = static_cast<int>(std::round(static_cast<double>(usage) * static_cast<double>(contextSize)));
        label.setTooltip(juce::translate("Context usage: USEDTK / CTXSIZE tokens").replace("USEDTK", juce::String(usedTokens)).replace("CTXSIZE", juce::String(contextSize)));
    }
    else
    {
        label.setTooltip(juce::translate("Context usage"));
    }
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
    // clang-format on
    return wildcards.joinIntoString(";");
}

static bool isInterestedInFile(juce::String const& filePath)
{
    auto const file = juce::File(filePath);
    return file.existsAsFile() && juce::WildcardFileFilter(getAllWildCards(), {}, {}).isFileSuitable(file);
}

// The header of the attached files that precedes the user query in the prompt
static auto constexpr attachedFilesHeader = "Attached files:\n";
static auto constexpr userQueryHeader = "User query:\n";

// Gets the question and the suggested answers from the arguments of the MCP tool elicit
static juce::String getElicitationQuestion(std::string const& arguments)
{
    try
    {
        auto const json = nlohmann::json::parse(arguments);
        auto question = juce::String::fromUTF8(json.value("message", std::string{}).c_str()).trim();
        if(question.isNotEmpty() && json.contains("options") && json.at("options").is_array())
        {
            for(auto const& option : json.at("options"))
            {
                if(option.is_string())
                {
                    question << "\n- " << juce::String::fromUTF8(option.get<std::string>().c_str());
                }
            }
        }
        return question;
    }
    catch(...)
    {
        return {};
    }
}

// Gets the answer of the user from the message of the MCP tool elicit (the text of the response, see Mcp::Handlers::elicit()):
// the remote agent keeps the JSON response of the tool while the local agent formats it as text
static std::optional<juce::String> getElicitationAnswer(common_chat_msg const& message)
{
    try
    {
        auto const response = nlohmann::json::parse(message.content);
        if(response.value("isError", true) || !response.contains("content") || !response.at("content").is_array() || response.at("content").empty())
        {
            return {};
        }
        return juce::String::fromUTF8(response.at("content").at(0).value("text", std::string{}).c_str());
    }
    catch(...)
    {
    }
    static auto constexpr resultHeader = "- Status: SUCCESSFUL\n- Result:\n";
    auto const content = juce::String::fromUTF8(message.content.c_str());
    if(!content.contains(resultHeader))
    {
        return {};
    }
    return content.fromFirstOccurrenceOf(resultHeader, false, false).trimEnd();
}

Application::Neuralyzer::Chat::Chat(BackgroundAgent& agent)
: mAgent(agent)
, mSendButton(ScalableImage::fromSvg(AnlIconsData::Stop_svg, AnlIconsData::Stop_svgSize), ScalableImage::fromSvg(AnlIconsData::Send_svg, AnlIconsData::Send_svgSize))
, mAttachedFilesButton(ScalableImage::fromSvg(AnlIconsData::Attachment_svg, AnlIconsData::Attachment_svgSize))
{
    mQueryEditor.setMultiLine(true);
    mQueryEditor.setReturnKeyStartsNewLine(false);
    mQueryEditor.setScrollbarsShown(true);
    mQueryEditor.setCaretVisible(true);
    mQueryEditor.setPopupMenuEnabled(true);
    mQueryEditor.setFont(juce::Font(juce::FontOptions(14.0f)));
    mQueryEditor.onReturnKey = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        if(mElicitation.has_value())
        {
            if(hasQuery)
            {
                sendElicitationAnswer();
            }
            return;
        }
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
        updateSendButton();
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
            m.addItem(juce::translate("Detach FLNAME").replace("FLNAME", juce::File(file).getFileName()), [=, this]()
                      {
                          mDroppedFilePaths.removeString(file);
                          updateAttachedFilesButton();
                      });
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mAttachedFilesButton).withDeletionCheck(*this));
    };

    // The panel is updated asynchronously when the agent resumes the query (a button cannot delete itself on click)
    mElicitationPanel.onOptionClicked = [this](juce::String const& answer)
    {
        mAgent.answerElicitation(answer);
    };

    mHistoryEditor.onShowPopupMenu = [this](juce::PopupMenu& menu)
    {
        menu.addSeparator();
        auto const canReset = mAgent.getModelInfo().isValid();
        menu.addItem(juce::translate("Reset History"), canReset, false, [this]()
                     {
                         auto const options = juce::MessageBoxOptions()
                                                  .withIconType(juce::AlertWindow::QuestionIcon)
                                                  .withTitle(juce::translate("Reset the agent history?"))
                                                  .withMessage(juce::translate("Are you sure you want to reset the agent history? This will clear the current chat history and temporary response."))
                                                  .withButton(juce::translate("Ok"), 1)
                                                  .withButton(juce::translate("Cancel"), 0)
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
        else if(mElicitation.has_value() && mQueryEditor.getText().trim().isNotEmpty())
        {
            sendElicitationAnswer();
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
    // The state of the button follows the action of the agent (see updateSendButton())
    mSendButton.setClickingTogglesState(false);

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
    addChildComponent(mElicitationPanel);
    addAndMakeVisible(mQueryEditor);
    addAndMakeVisible(mSendButton);
    addAndMakeVisible(mAttachedFilesButton);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);
    addAndMakeVisible(mUsageLabel);

    mAgent.addChangeListener(this);
    updateAttachedFilesButton();
    updated();
}

Application::Neuralyzer::Chat::~Chat()
{
    mAgent.removeChangeListener(this);
    mQueryEditor.removeMouseListener(this);
}

void Application::Neuralyzer::Chat::updateAttachedFilesButton()
{
    auto const files = mDroppedFilePaths.isEmpty() ? juce::translate("None") : mDroppedFilePaths.joinIntoString(", ");
    mAttachedFilesButton.setTooltip(juce::translate("Attached files: FILEPATHS").replace("FILEPATHS", files));
    mAttachedFilesButton.setToggleState(!mDroppedFilePaths.isEmpty(), juce::NotificationType::dontSendNotification);
}

void Application::Neuralyzer::Chat::updateSendButton()
{
    auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
    auto const currentAction = mAgent.getCurrentAction();
    auto const isRunningQuery = currentAction == BackgroundAgent::Action::sendQuery;
    auto const isIdle = currentAction == BackgroundAgent::Action::none;
    auto const isInitialized = mAgent.getModelInfo().isValid();
    // While the agent waits for the answer to its question, the button sends the answer if there is one and stops the query otherwise
    auto const isAnswering = mElicitation.has_value() && hasQuery;
    auto const canStop = isRunningQuery && !isAnswering;
    mSendButton.setEnabled(isInitialized && (isRunningQuery || (isIdle && hasQuery)));
    mSendButton.setTooltip(isAnswering ? juce::translate("Send Answer") : (isRunningQuery ? juce::translate("Stop Query") : juce::translate("Send Query")));
    // The size of the button depends on its state (see resized())
    if(mSendButton.getToggleState() == canStop)
    {
        mSendButton.setToggleState(!canStop, juce::NotificationType::dontSendNotification);
        resized();
    }
}

void Application::Neuralyzer::Chat::updateElicitation()
{
    auto const elicitation = mAgent.getPendingElicitation();
    if(elicitation == mElicitation)
    {
        return;
    }
    mElicitation = elicitation;
    auto const options = mElicitation.has_value() ? mElicitation.value().options : juce::StringArray{};
    mElicitationPanel.setOptions(options);
    mElicitationPanel.setVisible(!options.isEmpty());
    if(!mElicitation.has_value())
    {
        return;
    }
    if(mQueryEditor.isShowing())
    {
        mQueryEditor.grabKeyboardFocus();
    }
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
    // The images fill the bounds of the icons whatever the viewBox of the SVG, so the stop button is reduced to keep the
    // filled square lighter than the send arrow
    auto const sendBounds = menuArea.removeFromRight(18);
    mSendButton.setBounds(mSendButton.getToggleState() ? sendBounds : sendBounds.withSizeKeepingCentre(12, 12));
    mAttachedFilesButton.setBounds(menuArea.removeFromLeft(18));
    auto inputBounds = bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 7).reduced(4, 0);
    if(mElicitationPanel.isVisible())
    {
        // The suggested answers take the space of the query editor that keeps a minimum height (the answers can be scrolled)
        static auto constexpr margin = 4;
        auto const minQueryHeight = PropertyComponentBase::defaultHeight * 2;
        auto const maxPanelHeight = std::max(inputBounds.getHeight() - minQueryHeight - margin * 2, 0);
        auto const panelHeight = std::min(mElicitationPanel.getOptimalHeight(inputBounds.getWidth()), maxPanelHeight);
        inputBounds.removeFromTop(margin);
        mElicitationPanel.setBounds(inputBounds.removeFromTop(panelHeight));
        inputBounds.removeFromTop(margin);
    }
    mQueryEditor.setBounds(inputBounds);
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

    auto const useThinkingMode = mAgent.getAccessor().getAttr<AttrType::enableThinking>();
    // Adds the file paths to the query
    if(!mDroppedFilePaths.isEmpty())
    {
        juce::String fullQuery;
        fullQuery += attachedFilesHeader;
        for(auto const& file : mDroppedFilePaths)
        {
            fullQuery += "- " + file + "\n";
        }
        fullQuery += "\n";
        // Adds the user request to the query
        fullQuery += userQueryHeader + query;
        mAgent.sendQuery(fullQuery, useThinkingMode);
    }
    else
    {
        mAgent.sendQuery(query, useThinkingMode);
    }

    // The files are attached to this query only
    mDroppedFilePaths.clearQuick();
    updateAttachedFilesButton();

    mTempResponse.setText(juce::translate("Processing query"), false);
    mTempResponse.setVisible(true);
    mSeparator2.setVisible(true);
    startTimer(250);
    updated();
    resized();
}

void Application::Neuralyzer::Chat::sendElicitationAnswer()
{
    auto const answer = mQueryEditor.getText().trim();
    MiscWeakAssert(mElicitation.has_value() && answer.isNotEmpty());
    if(!mElicitation.has_value() || answer.isEmpty())
    {
        return;
    }

    // The attached files are kept for the next query
    mQueryEditor.setText({}, true);
    mQueryEditor.resetHistoryIndex();
    mAgent.answerElicitation(answer);
    updated();
}

void Application::Neuralyzer::Chat::stopUserQuery()
{
    MiscWeakAssert(mAgent.getCurrentAction() == BackgroundAgent::Action::sendQuery);
    if(mAgent.getCurrentAction() != BackgroundAgent::Action::sendQuery)
    {
        return;
    }

    mAgent.cancelQuery();
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

    mAgent.startSession();
    startTimer(250);
    updated();
}

void Application::Neuralyzer::Chat::updateHistory()
{
    getTempHistoryFile().create();
    juce::FileOutputStream fullHistoryStream(getTempHistoryFile());
    MiscWeakAssert(fullHistoryStream.openedOk());
    if(fullHistoryStream.openedOk())
    {
        fullHistoryStream.setPosition(0);
        fullHistoryStream.truncate();
    }

    History tempHistory;
    auto const messages = mAgent.getHistory();
    tempHistory.reserve(messages.size());
    for(auto const& message : messages)
    {
        auto content = juce::String(message.render_content());
        if(content.isEmpty() && !message.reasoning_content.empty())
        {
            content << "> " << juce::String(message.reasoning_content);
        }
        if(!message.tool_calls.empty())
        {
            content << "\n**Tool Call(s)**:\n";
            for(auto const& toolCall : message.tool_calls)
            {
                content << "\n- " << toolCall.name << ": *" << toolCall.arguments << "*";
            }
        }
        if(content.isNotEmpty())
        {
            auto const role = magic_enum::enum_cast<MessageType>(message.role);
            tempHistory.push_back({role.value_or(MessageType::assistant), std::move(content)});
        }
        // The questions of the model and the answers of the user (see the MCP tool elicit) are displayed as messages
        for(auto const& toolCall : message.tool_calls)
        {
            auto question = toolCall.name == "elicit" ? getElicitationQuestion(toolCall.arguments) : juce::String{};
            if(question.isNotEmpty())
            {
                tempHistory.push_back({MessageType::assistant, std::move(question)});
            }
        }
        if(message.role == "tool" && message.tool_name == "elicit")
        {
            auto answer = getElicitationAnswer(message);
            if(answer.has_value())
            {
                tempHistory.push_back({MessageType::user, std::move(answer.value())});
            }
        }
    }

    History history;
    history.emplace_back(MessageType::assistant, juce::translate("I'm your AI agent. How can I help you today?"));
    for(auto const& message : tempHistory)
    {
        if(fullHistoryStream.openedOk())
        {
            static auto const lineBreak = "  \n";
            fullHistoryStream << juce::String(std::string(magic_enum::enum_name(message.first))) << ":" << lineBreak;
            fullHistoryStream << message.second << lineBreak << lineBreak;
            fullHistoryStream << "---" << lineBreak;
        }

        if(message.first == MessageType::assistant)
        {
            if(history.back().first != MessageType::assistant)
            {
                history.push_back(message);
            }
            else
            {
                history.back().second = message.second;
            }
        }
        else if(message.first == MessageType::user)
        {
            // Only the user query is displayed, without the header of the attached files
            auto const& text = message.second;
            history.push_back({MessageType::user, text.startsWith(attachedFilesHeader) ? text.fromFirstOccurrenceOf(userQueryHeader, false, false) : text});
        }
        else if(message.first == MessageType::warning)
        {
            history.push_back(message);
        }
    }

    auto const lastResult = mAgent.getLastResult();
    if(lastResult.failed())
    {
        history.push_back({MessageType::warning, lastResult.getErrorMessage()});
    }
    auto const startupDecision = mAgent.getStartupDecision();
    if(startupDecision == BackgroundAgent::StartupDecision::wait)
    {
        history.push_back({MessageType::warning, juce::translate("The agent crashed during the previous session. It is waiting for your decision before starting.")});
    }
    else if(startupDecision == BackgroundAgent::StartupDecision::ignore)
    {
        history.push_back({MessageType::warning, juce::translate("The agent is disabled. Restart the application to relaunch it.")});
    }
    mHistoryEditor.setHistory(history);
    mQueryEditor.setHistory(history);
}

void Application::Neuralyzer::Chat::updated()
{
    updateElicitation();
    timerCallback();

    auto const modelInfo = mAgent.getModelInfo();
    auto const currentAction = mAgent.getCurrentAction();
    auto const startupDecision = mAgent.getStartupDecision();
    if(startupDecision == BackgroundAgent::StartupDecision::wait)
    {
        auto const message = juce::translate("The agent crashed during the previous session. It is waiting for your decision before starting.");
        mStatusLabel.setText(message, juce::dontSendNotification);
        mQueryEditor.setTextToShowWhenEmpty(message, findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    else if(startupDecision == BackgroundAgent::StartupDecision::ignore)
    {
        auto const message = juce::translate("The agent is disabled. Restart the application to relaunch it.");
        mStatusLabel.setText(message, juce::dontSendNotification);
        mQueryEditor.setTextToShowWhenEmpty(message, findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
        // The whole component is disabled: the settings button is handled by the NeuralyzerPanel and remains accessible.
        setEnabled(false);
    }
    else if(!isConfigured())
    {
        mStatusLabel.setText(juce::translate("Configure the agent in the settings panel."), juce::dontSendNotification);
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("The agent is not configured"), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    else if(currentAction == BackgroundAgent::Action::none)
    {
        // The temporary response remains visible if the query is pending
        if(!mAgent.hasPendingAction())
        {
            stopTimer();
            mTempResponse.setVisible(false);
            mSeparator2.setVisible(false);
        }
        auto const result = mAgent.getLastResult();
        if(result.failed())
        {
            mStatusLabel.setText(juce::translate("Error: ERRSTR").replace("ERRSTR", result.getErrorMessage()), juce::dontSendNotification);
            mQueryEditor.setTextToShowWhenEmpty(juce::translate("Error: ERRSTR").replace("ERRSTR", result.getErrorMessage()), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
        }
        else if(modelInfo.isValid())
        {
            mStatusLabel.setText(juce::translate("Enter your query in natural language..."), juce::dontSendNotification);
            mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
        }
    }
    else
    {
        if(!isTimerRunning())
        {
            startTimer(250);
        }
        if(currentAction == BackgroundAgent::Action::sendQuery)
        {
            // The question is displayed in the history and the suggested answers replace the temporary response
            mTempResponse.setVisible(!mElicitation.has_value());
            mSeparator2.setVisible(!mElicitation.has_value());
        }
        auto const placeholder = mElicitation.has_value() ? juce::translate("Answer the question of the agent...") : juce::translate("Enter your query in natural language...");
        mQueryEditor.setTextToShowWhenEmpty(placeholder, findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }

    updateUsageLabel(mUsageLabel, mAgent);

    updateSendButton();
    updateHistory();
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
            case BackgroundAgent::Action::initializeRag:
            {
                return juce::translate("Initializing documentation search");
            }
            case BackgroundAgent::Action::updateRagIndex:
            {
                return juce::translate("Indexing documentation");
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
                return mElicitation.has_value() ? juce::translate("Waiting for your answer") : juce::translate("Processing query");
            }
            case BackgroundAgent::Action::saveSession:
            {
                return juce::translate("Saving session");
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
    updateUsageLabel(mUsageLabel, mAgent);
}

void Application::Neuralyzer::Chat::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    updated();
}

ANALYSE_FILE_END
