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

Application::Neuralyzer::Chat::Chat(Accessor& accessor, Agent& agent)
: mAccessor(accessor)
, mAgent(agent)
, mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::stop_png, AnlIconsData::stop_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::send_png, AnlIconsData::send_pngSize))
{
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
        auto const isRunningQuery = mRequestFuture.valid();
        auto const canSendUserQuery = mIsInitialized && hasQuery && !isRunningQuery;
        if(canSendUserQuery)
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
        auto const isRunningQuery = mRequestFuture.valid();
        auto const canSendOrStopUserQuery = mIsInitialized && (isRunningQuery || hasQuery);
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
        auto const canReset = mAccessor.getAttr<AttrType::modelInfo>().valid() || mIsInitialized;
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
                                                          if(mAccessor.getAttr<AttrType::modelInfo>().valid() || mIsInitialized)
                                                          {
                                                              clearHistory();
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
                if(acsr.getAttr<AttrType::modelInfo>().valid() || mIsInitialized)
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
        if(!mIsInitialized && !mAccessor.getAttr<AttrType::modelInfo>().valid())
        {
            askToConfigureNeuralyzer();
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
    auto statusBounds = bounds.removeFromBottom(20);
    mUsageLabel.setBounds(statusBounds.removeFromRight(40));
    mStatusLabel.setBounds(statusBounds);
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
    auto const isInitialized = mIsInitialized;
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

        auto [result, responses] = mRequestFuture.get();
        mAgent.setShouldQuit(false);
        if(result.failed())
        {
            responses.push_back(result.getErrorMessage());
            mHistory.push_back(std::make_tuple(MessageType::error, std::move(responses)));
            mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", result.getErrorMessage()), juce::dontSendNotification);
        }
        else
        {
            mStatusLabel.setText(juce::translate("Please enter a query"), juce::dontSendNotification);
            if(responses.empty())
            {
                responses.push_back(juce::translate("No response from the model."));
                mHistory.push_back(std::make_tuple(MessageType::error, std::move(responses)));
                mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", juce::translate("No response from the model.")), juce::dontSendNotification);
            }
            else
            {
                mHistory.push_back(std::make_tuple(MessageType::assistant, std::move(responses)));
            }
        }
    }
    auto const modelInfo = mAgent.getModelInfo();
    mIsInitialized = modelInfo.valid();
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
        if(modelInfo.contextSize.has_value())
        {
            modelInfoParts.add(juce::translate("Context Size: SIZE").replace("SIZE", juce::String(modelInfo.batchSize.value())));
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
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Send Query"));
    auto const hasQuery = mQueryEditor.getText().trim().isNotEmpty();
    mSendButton.setEnabled(mIsInitialized && hasQuery);
    colourChanged();
    resized();
}

void Application::Neuralyzer::Chat::initializeSystem()
{
    if(mRequestFuture.valid())
    {
        mAgent.setShouldQuit(true);
        auto const state = mRequestFuture.wait_for(std::chrono::milliseconds(100));
        cancelPendingUpdate();
        if(state != std::future_status::ready)
        {
            juce::MessageManager::callAsync([this, weakReference = juce::WeakReference<juce::Component>(this)]()
                                            {
                                                if(weakReference.get() != nullptr)
                                                {
                                                    initializeSystem();
                                                }
                                            });
            return;
        }
        mRequestFuture.get();
        mAgent.setShouldQuit(false);
    }
    MiscWeakAssert(!mRequestFuture.valid());
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    colourChanged();
    resized();

    mIsInitialized = false;
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
    MiscWeakAssert(info.valid());
    if(!info.valid())
    {
        return;
    }

    mRequestFuture = std::async(std::launch::async, [=, this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::Chat::Initialize");
                                    MiscDebug("Application::Neuralyzer::Chat", "Initializing Neuralyzer chat system...");
                                    auto result = [&]()
                                    {
                                        Chrono chrono{"Neuralyzer"};
                                        // Initialize the model
                                        chrono.start();
                                        static auto const instructions = juce::String(juce::CharPointer_UTF8(AnlNeuralyzerData::Instructions_md));
                                        auto initResult = mAgent.initialize(info, instructions);
                                        chrono.stop("Initialization ended");
                                        if(mAgent.shouldQuit())
                                        {
                                            return std::make_tuple(juce::Result::ok(), std::vector<juce::String>{});
                                        }
                                        MiscWeakAssert(initResult.wasOk());
                                        if(initResult.failed())
                                        {
                                            MiscDebug("Application::Neuralyzer::Chat", "Failed to initialize the model: " + initResult.getErrorMessage());
                                            return std::make_tuple(std::move(initResult), std::vector<juce::String>{});
                                        }

                                        // Send the introduction query
                                        chrono.start();
                                        auto queryResult = mAgent.sendUserQuery("Now, introduce yourself in one sentence.", false);
                                        chrono.stop("Introduction query ended");
                                        if(mAgent.shouldQuit())
                                        {
                                            return std::make_tuple(juce::Result::ok(), std::vector<juce::String>{});
                                        }
                                        MiscWeakAssert(std::get<0_z>(queryResult).wasOk());
                                        if(std::get<0_z>(queryResult).failed())
                                        {
                                            MiscDebug("Application::Neuralyzer::Chat", "Failed to query the introduction: " + std::get<0_z>(queryResult).getErrorMessage());
                                            return queryResult;
                                        }
                                        MiscDebug("Application::Neuralyzer::Chat", "Neuralyzer chat system initialized");
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

    if(!mIsInitialized)
    {
        askToConfigureNeuralyzer();
        return;
    };

    mQueryEditor.setText({}, true);
    mQueryEditor.resetHistoryIndex();
    mStatusLabel.setText(juce::translate("Processing query"), juce::dontSendNotification);
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mHistory.push_back(std::make_tuple(MessageType::user, std::vector<juce::String>{query}));
    MiscWeakAssert(!mAgent.shouldQuit());
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::Chat::Request");
                                    Chrono chrono{"Neuralyzer"};
                                    chrono.start();
                                    auto result = mAgent.sendUserQuery(query, true);
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
        mRequestFuture.wait();
        cancelPendingUpdate();
        mRequestFuture.get();
        mAgent.setShouldQuit(false);
        mStatusLabel.setText(juce::translate("Query aborted"), juce::dontSendNotification);
        mHistory.push_back(std::make_tuple(MessageType::error, std::vector<juce::String>{juce::translate("Query aborted by user.")}));
    }
    MiscWeakAssert(!mRequestFuture.valid());
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    colourChanged();
    resized();
}

void Application::Neuralyzer::Chat::clearHistory()
{
    MiscWeakAssert(mIsInitialized);
    if(!mIsInitialized)
    {
        return;
    };

    stopUserQuery();

    mIsInitialized = false;
    mSendButton.setEnabled(true);
    mSendButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(false);
    mHistory.clear();
    updateHistory();
    mStatusLabel.setText(juce::translate("Initializing..."), juce::dontSendNotification);
    mQueryEditor.setTextToShowWhenEmpty({}, findColour(juce::TextButton::ColourIds::buttonOnColourId));

    MiscWeakAssert(!mAgent.shouldQuit());
    MiscWeakAssert(!mRequestFuture.valid());

    mRequestFuture = std::async(std::launch::async, [this]()
                                {
                                    juce::Thread::setCurrentThreadName("Neuralyzer::Chat::Reset");
                                    Chrono chrono{"Neuralyzer"};
                                    mAgent.resetState();
                                    chrono.start();
                                    auto queryResult = mAgent.sendUserQuery("Now, introduce yourself in one sentence.", false);
                                    chrono.stop("Introduction query ended");
                                    MiscWeakAssert(std::get<0_z>(queryResult).wasOk());
                                    triggerAsyncUpdate();
                                    return queryResult;
                                });
    startTimer(250);
}

void Application::Neuralyzer::Chat::updateHistory()
{
    auto const userColour = findColour(juce::TextEditor::ColourIds::textColourId);
    auto const font = juce::Font(juce::FontOptions(14.0f));
    mHistoryEditor.clear();
    for(auto const& [role, request] : mHistory)
    {
        juce::String roleStr;
        switch(role)
        {
            case MessageType::user:
                roleStr = juce::String(juce::CharPointer_UTF8("\xf0\x9f\x91\xa4 ")) + juce::translate("User"); // 👤
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, userColour);
                break;
            case MessageType::assistant:
                roleStr = juce::String(juce::CharPointer_UTF8("\xf0\x9f\xa4\x96 ")) + juce::translate("Assistant"); // 🤖
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colours::green);
                break;
            case MessageType::error:
                roleStr = juce::String(juce::CharPointer_UTF8("\xe2\x9a\xa0\xef\xb8\x8f ")) + juce::translate("Error"); // ⚠️
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colours::red);
                break;
        }
        mHistoryEditor.setFont(font.boldened());
        mHistoryEditor.insertTextAtCaret(roleStr + ":\n");
        mHistoryEditor.setFont(font);
        Formatter::addMarkdown(mHistoryEditor, request.empty() ? "" : request.back().trim());
        mHistoryEditor.insertTextAtCaret("\n");
    }
    mHistoryEditor.scrollDown();
}

void Application::Neuralyzer::Chat::timerCallback()
{
    auto const text = mStatusLabel.getText();
    // Find the number of dots at the end of the text
    auto const index = text.indexOf(".");
    auto const numDots = (index >= 0) ? text.length() - index : 0;
    juce::String newText = mIsInitialized ? juce::translate("Processing query") : juce::translate("Initializing");
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
    mUsageLabel.setText(juce::String(static_cast<int>(mAgent.getContextCapacityUsage() * 100.0f)) + "%", juce::dontSendNotification);
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
                    auto const& messages = std::get<1_z>(*historyIt);
                    setText(messages.empty() ? "" : messages.back(), true);
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
                        auto const& messages = std::get<1_z>(*historyIt);
                        setText(messages.empty() ? "" : messages.back(), true);
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
    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    return std::any_of(files.begin(), files.end(), [&](auto const& fileName)
                       {
                           return audioFormatsWildcard.containsIgnoreCase(juce::File(fileName).getFileExtension());
                       });
}

void Application::Neuralyzer::Chat::QueryEditor::fileDragEnter([[maybe_unused]] juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
}

void Application::Neuralyzer::Chat::QueryEditor::fileDragExit([[maybe_unused]] juce::StringArray const& files)
{
}

void Application::Neuralyzer::Chat::QueryEditor::filesDropped(juce::StringArray const& files, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    auto const audioFormatsWildcard = Instance::getWildCardForAudioFormats();
    for(auto const& file : files)
    {
        if(audioFormatsWildcard.containsIgnoreCase(juce::File(file).getFileExtension()))
        {
            JUCE_COMPILER_WARNING("implement that");
        }
    }
}

ANALYSE_FILE_END
