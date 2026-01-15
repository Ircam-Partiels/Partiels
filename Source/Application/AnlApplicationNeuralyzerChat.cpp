#include "AnlApplicationNeuralyzerChat.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

static void askToLoadAModel()
{
    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("Neuralyzer Model Not Set"))
                             .withMessage(juce::translate("The Neuralyzer model is not set. Do you want to open the Neuralyzer settings panel to set it now?"))
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

Application::Neuralyzer::Chat::Chat(Accessor& accessor, Agent& agent)
: mAccessor(accessor)
, mAgent(agent)
, mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::stop_png, AnlIconsData::stop_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::send_png, AnlIconsData::send_pngSize))
{
    mHistoryEditor.setReadOnly(true);
    mHistoryEditor.setMultiLine(true);
    mHistoryEditor.setScrollbarsShown(true);
    mHistoryEditor.setCaretVisible(true);
    mHistoryEditor.setPopupMenuEnabled(true);
    mHistoryEditor.setFont(juce::Font(juce::FontOptions(14.0f)));

    mQueryEditor.setMultiLine(true);
    mQueryEditor.setReturnKeyStartsNewLine(false);
    mQueryEditor.setScrollbarsShown(true);
    mQueryEditor.setCaretVisible(true);
    mQueryEditor.setPopupMenuEnabled(true);
    mQueryEditor.setFont(juce::Font(juce::FontOptions(14.0f)));
    mQueryEditor.onReturnKey = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        auto const isRunningQuery = mRequestFuture.valid();
        auto const canSendUserQuery = mIsInitialized.load() && hasQuery && !isRunningQuery;
        if(canSendUserQuery)
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        auto const isRunningQuery = mRequestFuture.valid();
        auto const canSendOrStopUserQuery = mIsInitialized.load() && (isRunningQuery || hasQuery);
        mSendButton.setEnabled(canSendOrStopUserQuery);
    };
    mQueryEditor.addMouseListener(this, true);

    // Create a triangle
    {
        juce::DrawablePath trianglePath;
        juce::Path p;
        p.addTriangle({0, 0}, {10, 0}, {5, 10});
        trianglePath.setPath(p);
        mMenuButton.setImages(&trianglePath);
        mMenuButton.setEdgeIndent(1);
    }
    mMenuButton.onClick = [this]()
    {
        juce::PopupMenu menu;
        auto const canReset = mAccessor.getAttr<AttrType::modelInfo>().model != juce::File{} || mIsInitialized.load();
        menu.addItem(juce::translate("Reset Model"), canReset, false, [this]()
                     {
                         auto const options = juce::MessageBoxOptions()
                                                  .withIconType(juce::AlertWindow::QuestionIcon)
                                                  .withTitle(juce::translate("Reset the Neuralyzer Model?"))
                                                  .withMessage(juce::translate("Are you sure you want to reset the Neuralyzer model? This will clear the current chat history and temporary response."))
                                                  .withButton(juce::translate("Ok"))
                                                  .withButton(juce::translate("Cancel"))

                                                  .withAssociatedComponent(this);
                         juce::AlertWindow::showAsync(options, [this](int result)
                                                      {
                                                          if(result != 1)
                                                          {
                                                              return;
                                                          }
                                                          if(mAccessor.getAttr<AttrType::modelInfo>().model.existsAsFile() || mIsInitialized.load())
                                                          {
                                                              initializeSystem();
                                                          }
                                                      });
                     });
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mMenuButton).withDeletionCheck(*this));
    };
    mSendButton.onClick = [this]()
    {
        if(mRequestFuture.valid())
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
    addAndMakeVisible(mMenuButton);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelInfo:
            {
                if(acsr.getAttr<AttrType::modelInfo>().model.existsAsFile() || mIsInitialized.load())
                {
                    initializeSystem();
                }
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::Neuralyzer::Chat::~Chat()
{
    mQueryEditor.removeMouseListener(this);
    mAccessor.removeListener(mListener);
    stopUserQuery();
}

void Application::Neuralyzer::Chat::mouseDown(juce::MouseEvent const& event)
{
    if(event.eventComponent == &mQueryEditor)
    {
        if(!mIsInitialized.load() && mAccessor.getAttr<AttrType::modelInfo>().model == juce::File{})
        {
            askToLoadAModel();
        }
    }
}

void Application::Neuralyzer::Chat::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::Neuralyzer::Chat::resized()
{
    auto bounds = getLocalBounds();
    mStatusLabel.setBounds(bounds.removeFromBottom(20));
    mSeparator3.setBounds(bounds.removeFromBottom(1));
    mQueryEditor.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 7).reduced(4, 0));
    mSendButton.setBounds(mQueryEditor.getBounds().removeFromBottom(22).withTrimmedRight(4).removeFromRight(18));
    mMenuButton.setBounds(mQueryEditor.getBounds().expanded(4, 0).removeFromBottom(8).removeFromRight(8));
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
    auto const isInitialized = mIsInitialized.load();
    auto const isRunningQuery = mRequestFuture.valid();
    if(!isInitialized && !isRunningQuery)
    {
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Select a model in the settings panel."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    else
    {
        mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    }
    updateHistory();
}

void Application::Neuralyzer::Chat::parentHierarchyChanged()
{
    colourChanged();
}

void Application::Neuralyzer::Chat::handleAsyncUpdate()
{
    MiscWeakAssert(mRequestFuture.valid());
    if(mRequestFuture.valid())
    {
        auto const state = mRequestFuture.wait_for(std::chrono::milliseconds(0));
        MiscWeakAssert(state == std::future_status::ready);
        if(state != std::future_status::ready)
        {
            triggerAsyncUpdate();
            return;
        }

        auto const result = mRequestFuture.get();
        mAgent.setShouldQuit(false);
        if(std::get<0>(result).failed())
        {
            auto const error = std::get<0>(result).getErrorMessage();
            mHistory.push_back(std::make_tuple(MessageType::error, error));
            mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", error), juce::dontSendNotification);
        }
        else
        {
            mStatusLabel.setText(juce::translate("Please enter a query"), juce::dontSendNotification);
            auto response = std::get<1>(result);
            if(response.empty())
            {
                mHistory.push_back(std::make_tuple(MessageType::error, juce::translate("No response from the model.")));
                mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", juce::translate("No response from the model.")), juce::dontSendNotification);
            }
            else
            {
                mHistory.push_back(std::make_tuple(MessageType::assistant, response));
            }
        }
    }
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Send Query"));
    auto const isInitialized = mIsInitialized.load();
    auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
    mSendButton.setEnabled(isInitialized && hasQuery);
    colourChanged();
    resized();
}

void Application::Neuralyzer::Chat::initializeSystem()
{
    stopUserQuery();

    mIsInitialized.store(false);
    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);
    mHistory.clear();
    updateHistory();
    mStatusLabel.setText(juce::translate("Initializing..."), juce::dontSendNotification);
    mQueryEditor.setTextToShowWhenEmpty({}, findColour(juce::TextButton::ColourIds::buttonOnColourId));

    MiscWeakAssert(!mAgent.shouldQuit());
    MiscWeakAssert(!mRequestFuture.valid());

    auto info = mAccessor.getAttr<AttrType::modelInfo>();
    MiscWeakAssert(info.model.existsAsFile());
    if(!info.model.existsAsFile())
    {
        return;
    }

    static auto const instructions = [this]()
    {
        nlohmann::json request;
        request["method"] = "tools/call";
        nlohmann::json params;
        params["name"] = "get_installed_vamp_plugins_list";
        params["arguments"] = nlohmann::json::object();
        request["params"] = params;
        auto const response = mAgent.getMcpDispatcher().handle(request);
        MiscWeakAssert(response.contains("content") && response.at("content").is_array());
        if(!response.contains("content") || !response.at("content").is_array())
        {
            MiscDebug("Application::Neuralyzer::Chat", juce::String("Parsing error: ") + response.dump());
        }
        return juce::String(juce::CharPointer_UTF8(AnlNeuralyzerData::Instructions_md)).replace("PLUGINS_LIST", juce::String(response.at("content").dump())).trim();
    }();

    mRequestFuture = std::async(std::launch::async, [=, this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::Chat::Initialize");
                                    MiscDebug("Application::Neuralyzer::Chat", "Initializing Neuralyzer chat system...");
                                    auto result = [&]()
                                    {
                                        Chrono chrono{"Neuralyzer"};
                                        // Initialize the model
                                        chrono.start();
                                        auto initResult = mAgent.initialize(info, instructions);
                                        chrono.stop("Initialization ended");
                                        if(mAgent.shouldQuit())
                                        {
                                            return std::make_tuple(juce::Result::ok(), std::string{});
                                        }
                                        MiscWeakAssert(initResult.wasOk());
                                        if(initResult.failed())
                                        {
                                            MiscDebug("Application::Neuralyzer::Chat", "Failed to initialize the model: " + initResult.getErrorMessage());
                                            return std::make_tuple(std::move(initResult), std::string{});
                                        }

                                        // Send the introduction query
                                        chrono.start();
                                        auto queryResult = mAgent.sendUserQuery("Introduce yourself in one sentence.");
                                        chrono.stop("Introduction query ended");
                                        if(mAgent.shouldQuit())
                                        {
                                            return std::make_tuple(juce::Result::ok(), std::string{});
                                        }
                                        MiscWeakAssert(std::get<0_z>(queryResult).wasOk());
                                        if(std::get<0_z>(queryResult).failed())
                                        {
                                            MiscDebug("Application::Neuralyzer::Chat", "Failed to query the introduction: " + std::get<0_z>(queryResult).getErrorMessage());
                                            return queryResult;
                                        }
                                        MiscDebug("Application::Neuralyzer::Chat", "Neuralyzer chat system initialized");
                                        mIsInitialized.store(std::get<0_z>(queryResult).wasOk());
                                        return queryResult;
                                    }();
                                    triggerAsyncUpdate();
                                    return result;
                                });
    startTimer(250);
}

void Application::Neuralyzer::Chat::sendUserQuery()
{
    auto const query = mQueryEditor.getText().trim();
    MiscWeakAssert(query.isNotEmpty());
    MiscWeakAssert(!mRequestFuture.valid());
    if(mRequestFuture.valid() || query.isEmpty())
    {
        return;
    }

    if(!mIsInitialized.load())
    {
        askToLoadAModel();
        return;
    };

    mQueryEditor.setText({}, true);
    mQueryEditor.resetHistoryIndex();
    mStatusLabel.setText(juce::translate("Processing query"), juce::dontSendNotification);
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mAgent.clearTemporaryResponse();
    mHistory.push_back(std::make_tuple(MessageType::user, query));
    MiscWeakAssert(!mAgent.shouldQuit());
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::Chat::Request");
                                    Chrono chrono{"Neuralyzer"};
                                    chrono.start();
                                    auto result = mAgent.sendUserQuery(query);
                                    chrono.stop("Query processing ended");
                                    triggerAsyncUpdate();
                                    return result;
                                });
    mTempResponse.setText(juce::translate("Processing query"), false);
    mTempResponse.setVisible(true);
    mSeparator2.setVisible(true);
    startTimer(250);
    updateHistory();
    resized();
}

void Application::Neuralyzer::Chat::stopUserQuery()
{
    if(mRequestFuture.valid())
    {
        mAgent.setShouldQuit(true);
        cancelPendingUpdate();
        mRequestFuture.wait();
        mRequestFuture.get();
        mAgent.setShouldQuit(false);
        mStatusLabel.setText(juce::translate("Query aborted"), juce::dontSendNotification);
        mHistory.push_back(std::make_tuple(MessageType::error, juce::translate("Query aborted by user.")));
    }
    MiscWeakAssert(!mRequestFuture.valid());
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    colourChanged();
    resized();
}

void Application::Neuralyzer::Chat::updateHistory()
{
    auto const userColour = findColour(juce::TextEditor::ColourIds::textColourId);
    mHistoryEditor.clear();
    for(auto const& [role, request] : mHistory)
    {
        juce::String roleStr;
        switch(role)
        {
            case MessageType::user:
                roleStr = juce::translate("User");
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, userColour);
                break;
            case MessageType::assistant:
                roleStr = juce::translate("Assistant");
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colours::green);
                break;
            case MessageType::error:
                roleStr = juce::translate("Error");
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colours::red);
                break;
        }
        mHistoryEditor.insertTextAtCaret(roleStr + ": " + request.trim() + "\n\n");
        mHistoryEditor.scrollDown();
    }
}

void Application::Neuralyzer::Chat::timerCallback()
{
    auto const text = mStatusLabel.getText();
    // Find the number of dots at the end of the text
    auto const index = text.indexOf(".");
    auto const numDots = (index >= 0) ? text.length() - index : 0;
    juce::String newText = mIsInitialized.load() ? juce::translate("Processing query") : juce::translate("Initializing");
    for(auto i = 0; i < (numDots + 1) % 4; ++i)
    {
        newText += ".";
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

ANALYSE_FILE_END
