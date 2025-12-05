#include "AnlApplicationCoAnalyzer.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationInstance.h"
#include <AnlCoAnalyzerData.h>
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

static std::vector<juce::File> getInstalledModels()
{
    std::vector<juce::File> models;
    auto const addFilesFromDirectory = [&](juce::File const& root)
    {
#if JUCE_MAC
        auto const directory = root.getChildFile("Application Support").getChildFile("Ircam").getChildFile("partielsmodels");
#else
        auto const directory = root.getChildFile("Ircam").getChildFile("partielsmodels");
#endif
        auto const listedModels = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.gguf");
        for(auto const& model : listedModels)
        {
            if(std::none_of(models.cbegin(), models.cend(), [&](juce::File const& existingModel)
                            {
                                return existingModel.getFullPathName() == model.getFullPathName();
                            }))
            {
                models.push_back(model);
            }
        }
    };
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::commonApplicationDataDirectory));
    std::sort(models.begin(), models.end());
    return models;
}

juce::File Application::CoAnalyzer::getStateFile(Accessor const& accessor)
{
    auto const model = accessor.getAttr<AttrType::model>();
    auto const modelName = model.getFileNameWithoutExtension();
    auto const stateName = modelName + "_" + magic_enum::enum_name(accessor.getAttr<AttrType::instruction>()).data();
    return model.getSiblingFile(stateName).withFileExtension("bin");
}

Application::CoAnalyzer::SettingsContent::SettingsContent()
: mModel(juce::translate("Model"), juce::translate("The model used by the Neuralyzer"), "", {}, [this](size_t index)
         {
             if(index >= mInstalledModels.size())
             {
                 return;
             }
             auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
             accessor.setAttr<AttrType::model>(mInstalledModels.at(index), NotificationType::synchronous);
         })
, mInstruction(juce::translate("Instruction"), juce::translate("The instruction used by the Neuralyzer"), "", {}, [](size_t index)
               {
                   auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
                   accessor.setAttr<AttrType::instruction>(magic_enum::enum_cast<Instruction>(static_cast<int>(index)).value_or(Instruction::v2), NotificationType::synchronous);
               })
, mResetState(juce::translate("Reset State"), juce::translate("Reset the state of the model"), []()
              {
                  auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
                  if(getStateFile(accessor).deleteFile())
                  {
                      accessor.sendSignal(SignalType::fileUpdated, {}, NotificationType::synchronous);
                  }
              })
{
    addAndMakeVisible(mModel);
    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
    mModel.entry.setTextWhenNoChoicesAvailable(juce::translate("No model installed"));
    addAndMakeVisible(mInstruction);
    mInstruction.entry.addItem(juce::translate("v1 (long)"), 1);
    mInstruction.entry.addItem(juce::translate("v2 (short)"), 2);
    mInstruction.entry.setTextWhenNothingSelected(juce::translate("Select an instruction"));
    addAndMakeVisible(mResetState);

    auto const fileUpdated = [this](Accessor const& accessor, bool updateList)
    {
        if(updateList)
        {
            auto installedModels = getInstalledModels();
            if(installedModels != mInstalledModels)
            {
                mInstalledModels = std::move(installedModels);
                mModel.entry.clear(juce::NotificationType::dontSendNotification);
                for(auto const& model : mInstalledModels)
                {
                    mModel.entry.addItem(model.getFileName(), static_cast<int>(mModel.entry.getNumItems()) + 1);
                }
                mListener.onAttrChanged(accessor, AttrType::model);
            }
        }
        auto const model = accessor.getAttr<AttrType::model>();
        auto const it = std::find(mInstalledModels.cbegin(), mInstalledModels.cend(), model);
        if(it != mInstalledModels.cend() && it->existsAsFile())
        {
            mModel.entry.setSelectedId(static_cast<int>(std::distance(mInstalledModels.cbegin(), it)) + 1, juce::NotificationType::dontSendNotification);
        }
        else if(!model.existsAsFile())
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Model cannot be found"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        auto const instruction = accessor.getAttr<AttrType::instruction>();
        auto const index = magic_enum::enum_index(instruction).value_or(1);
        mInstruction.entry.setSelectedId(static_cast<int>(index) + 1, juce::NotificationType::dontSendNotification);
        mResetState.setEnabled(getStateFile(accessor).existsAsFile());
    };

    mListener.onAttrChanged = [=](Accessor const& accessor, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::model:
            case AttrType::instruction:
            {
                fileUpdated(accessor, false);
                break;
            }
        }
    };

    mReceiver.onSignal = [=](Accessor const& accessor, SignalType signal, juce::var)
    {
        switch(signal)
        {
            case SignalType::fileUpdated:
            {
                fileUpdated(accessor, true);
                break;
            }
        }
    };

    mTimerClock.callback = [=]()
    {
        auto const& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
        fileUpdated(accessor, true);
    };

    setSize(300, 200);
    auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
    fileUpdated(accessor, true);
    accessor.addListener(mListener, NotificationType::synchronous);
    accessor.addReceiver(mReceiver);
    mTimerClock.startTimer(500);
}

Application::CoAnalyzer::SettingsContent::~SettingsContent()
{
    mTimerClock.stopTimer();
    auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
    accessor.removeReceiver(mReceiver);
    accessor.removeListener(mListener);
}

void Application::CoAnalyzer::SettingsContent::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mModel);
    setBounds(mInstruction);
    setBounds(mResetState);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::CoAnalyzer::SettingsContent::parentHierarchyChanged()
{
    mTimerClock.callback();
}

void Application::CoAnalyzer::SettingsContent::visibilityChanged()
{
    mTimerClock.callback();
}

Application::CoAnalyzer::SettingsPanel::SettingsPanel()
{
    setContent(juce::translate("Neuralyzer Settings"), &mContent);
}

Application::CoAnalyzer::SettingsPanel::~SettingsPanel()
{
    setContent("", nullptr);
}

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
                                         window->getInterface().showCoAnalyzerSettingsPanel();
                                     }
                                 });
}

Application::CoAnalyzer::Chat::Chat(Accessor& accessor)
: mAccessor(accessor)
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
        if(mQueryEditor.getText().isNotEmpty())
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        mSendButton.setEnabled(mIsInitialized.load() && (mRequestFuture.valid() || mQueryEditor.getText().isNotEmpty()));
    };
    mQueryEditor.addMouseListener(this, true);
    mStatusLabel.setText(juce::translate("Model not intialized"), juce::dontSendNotification);

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
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);

    mListener.onAttrChanged = [this](Accessor const&, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::model:
            case AttrType::instruction:
            {
                if(mAccessor.getAttr<AttrType::model>() != juce::File{} || mIsInitialized.load())
                {
                    initializeSystem();
                }
                break;
            }
        }
    };

    mReceiver.onSignal = [this](Accessor const&, SignalType type, juce::var)
    {
        switch(type)
        {
            case SignalType::fileUpdated:
            {
                if(mAccessor.getAttr<AttrType::model>() != juce::File{} || mIsInitialized.load())
                {
                    initializeSystem();
                }
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.addReceiver(mReceiver);
}

Application::CoAnalyzer::Chat::~Chat()
{
    mQueryEditor.removeMouseListener(this);
    mAccessor.removeReceiver(mReceiver);
    mAccessor.removeListener(mListener);
    stopUserQuery();
}

void Application::CoAnalyzer::Chat::mouseDown(juce::MouseEvent const& event)
{
    if(event.eventComponent == &mQueryEditor)
    {
        if(!mIsInitialized.load() && mAccessor.getAttr<AttrType::model>() == juce::File{})
        {
            askToLoadAModel();
        }
    }
}

void Application::CoAnalyzer::Chat::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::CoAnalyzer::Chat::resized()
{
    auto bounds = getLocalBounds();
    mStatusLabel.setBounds(bounds.removeFromBottom(20));
    mSeparator3.setBounds(bounds.removeFromBottom(1));
    mQueryEditor.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 7).reduced(4, 0));
    mSendButton.setBounds(mQueryEditor.getBounds().removeFromRight(18).removeFromBottom(18));
    if(mTempResponse.isVisible())
    {
        mSeparator2.setBounds(bounds.removeFromBottom(1));
        mTempResponse.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 3).reduced(4, 0));
    }
    mSeparator1.setBounds(bounds.removeFromBottom(1));
    mHistoryEditor.setBounds(bounds.reduced(4, 0));
}

void Application::CoAnalyzer::Chat::colourChanged()
{
    auto const isInitialized = mIsInitialized.load();
    auto const isRunningQuery = mRequestFuture.valid();
    mSendButton.setToggleState(isInitialized && !isRunningQuery, juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(isInitialized && !isRunningQuery && mQueryEditor.getText().isNotEmpty());
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

void Application::CoAnalyzer::Chat::parentHierarchyChanged()
{
    colourChanged();
}

void Application::CoAnalyzer::Chat::handleAsyncUpdate()
{
    MiscWeakAssert(mRequestFuture.valid());
    if(!mRequestFuture.valid())
    {
        return;
    }

    auto const state = mRequestFuture.wait_for(std::chrono::milliseconds(0));
    MiscWeakAssert(state == std::future_status::ready);
    if(state != std::future_status::ready)
    {
        return;
    }

    auto const result = mRequestFuture.get();
    if(std::get<0>(result).failed())
    {
        auto const error = std::get<0>(result).getErrorMessage();
        mHistory.push_back(std::make_tuple(MessageType::error, error));
        mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", error), juce::dontSendNotification);
    }
    else
    {
        mStatusLabel.setText(juce::translate("Please enter a query"), juce::dontSendNotification);
        MiscWeakAssert(!std::get<1>(result).isNotEmpty());
        mHistory.push_back(std::make_tuple(MessageType::assistant, std::get<1>(result)));
        if(std::get<2>(result).isNotEmpty())
        {
            auto& documentAccessor = Instance::get().getDocumentAccessor();
            auto xml = juce::parseXML(std::get<2>(result));
            if(xml != nullptr)
            {
                Instance::get().getDocumentDirector().startAction();
                bool hasModification = false;
                for(auto* trackElement : xml->getChildWithTagNameIterator("tracks"))
                {
                    if(trackElement != nullptr && trackElement->hasAttribute("identifier"))
                    {
                        auto const identifier = trackElement->getStringAttribute("identifier");
                        if(Document::Tools::hasTrackAcsr(documentAccessor, identifier))
                        {
                            auto& srcAcsr = Document::Tools::getTrackAcsr(documentAccessor, identifier);
                            srcAcsr.fromXml(*trackElement, "tracks", NotificationType::synchronous);
                            hasModification = true;
                        }
                    }
                }
                if(hasModification && !Instance::get().getDocumentDirector().endAction(ActionState::newTransaction, juce::translate("Neuralyzer update document")))
                {
                    mStatusLabel.setText(juce::translate("Document modification failed"), juce::dontSendNotification);
                }
            }
        }
    }
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    mSendButton.setTooltip(juce::translate("Send Query"));
    colourChanged();
    resized();
}

void Application::CoAnalyzer::Chat::initializeSystem()
{
    stopUserQuery();

    mIsInitialized.store(false);
    mHistory.clear();
    updateHistory();
    mSendButton.setEnabled(false);
    mStatusLabel.setText(juce::translate("Initializing..."), juce::dontSendNotification);
    mQueryEditor.setTextToShowWhenEmpty({}, findColour(juce::TextButton::ColourIds::buttonOnColourId));

    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());

    auto const model = mAccessor.getAttr<AttrType::model>();
    auto const state = getStateFile(mAccessor);
    mRequestFuture = std::async(std::launch::async, [this, model, state]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Initialize");
                                    MiscDebug("Application::CoAnalyzer::Chat", "Initializing Neuralyzer chat system...");
                                    auto result = [&]()
                                    {
                                        Chrono chrono{"CoAnalyzer"};

                                        chrono.start();
                                        auto initializeResult = mChat.initialize(model);
                                        chrono.stop("Initialization ended");

                                        if(mShouldQuit.load())
                                        {
                                            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                        }

                                        MiscWeakAssert(initializeResult.wasOk());
                                        if(initializeResult.failed())
                                        {
                                            return std::make_tuple(std::move(initializeResult), juce::String{}, juce::String{});
                                        }
                                        // Load the saved state of the model
                                        if(state.existsAsFile())
                                        {
                                            chrono.start();
                                            auto stateResult = mChat.loadState(state);
                                            chrono.stop("State load ended");
                                            if(stateResult.failed())
                                            {
                                                MiscDebug("Application::CoAnalyzer::Chat", "Failed to inject state: " + stateResult.getErrorMessage());
                                                return std::make_tuple(std::move(stateResult), juce::String{}, juce::String{});
                                            }
                                        }
                                        else
                                        {
                                            chrono.start();
                                            auto systemMessageResult = mChat.addSystemMessage(juce::CharPointer_UTF8(AnlCoAnalyzerData::Instructions_v2_md));
                                            chrono.stop("State generation ended");
                                            if(systemMessageResult.failed())
                                            {
                                                MiscDebug("Application::CoAnalyzer::Chat", "Failed to initialize state: " + systemMessageResult.getErrorMessage());
                                                return std::make_tuple(std::move(systemMessageResult), juce::String{}, juce::String{});
                                            }
                                            mChat.saveState(state);
                                        }

                                        if(mShouldQuit.load())
                                        {
                                            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                        }

                                        chrono.start();
                                        auto userQueryResult = mChat.sendUserQuery("Introduce yourself in one sentence.");
                                        chrono.stop("Introduction generation ended");
                                        mIsInitialized.store(std::get<0_z>(userQueryResult).wasOk());
                                        return userQueryResult;
                                    }();
                                    triggerAsyncUpdate();
                                    return result;
                                });
    startTimer(250);
}

void Application::CoAnalyzer::Chat::sendUserQuery()
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
        askToLoadAModel();
        return;
    };

    // Get current document XML
    auto& documentAcsr = Instance::get().getDocumentAccessor();
    auto const items = Document::Selection::getItems(documentAcsr);
    if(std::get<1_z>(items).empty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Neuralyzer Document Selection Not Supported"))
                                 .withMessage(juce::translate("The Neuralyzer doesn't support group or document selection yet."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }
    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *std::get<1_z>(items).cbegin());
    auto xml = trackAcsr.toXml("tracks");
    xml->getChildByName("description")->getChildByName("output")->deleteAllChildElementsWithTagName("binNames");

    mQueryEditor.setText({}, juce::sendNotificationSync);
    mQueryEditor.resetHistoryIndex();
    mStatusLabel.setText(juce::translate("Processing query"), juce::dontSendNotification);
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mChat.clearTemporaryResponse();
    mHistory.push_back(std::make_tuple(MessageType::user, query));
    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query, xmld = std::move(xml)]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Request");
                                    auto result = [&]()
                                    {
                                        Chrono chrono{"CoAnalyzer"};
                                        static juce::String const dataTpl = juce::CharPointer_UTF8(AnlCoAnalyzerData::ContextTemplate_md);
                                        chrono.start();
                                        auto contextResult = mChat.injectContext(dataTpl.replace("FULL_XML_CONTEXT", xmld->toString()));
                                        chrono.stop("Context injection ended");
                                        if(contextResult.failed())
                                        {
                                            return std::make_tuple(std::move(contextResult), juce::String{}, juce::String{});
                                        }
                                        chrono.start();
                                        auto userQueryResult = mChat.sendUserQuery(query);
                                        chrono.stop("User query processing ended");
                                        return userQueryResult;
                                    }();
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

void Application::CoAnalyzer::Chat::stopUserQuery()
{
    if(mRequestFuture.valid())
    {
        mShouldQuit.store(true);
        cancelPendingUpdate();
        mRequestFuture.wait();
        mRequestFuture.get();
        mStatusLabel.setText(juce::translate("Query aborted"), juce::dontSendNotification);
        mHistory.push_back(std::make_tuple(MessageType::error, juce::translate("Query aborted by user.")));
    }
    mTempResponse.setVisible(false);
    mSeparator2.setVisible(false);
    stopTimer();
    mSendButton.setTooltip(juce::translate("Send Query"));
    colourChanged();
    resized();
}

void Application::CoAnalyzer::Chat::updateHistory()
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
    }
}

void Application::CoAnalyzer::Chat::timerCallback()
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
    auto const reponseText = mChat.getTemporaryResponse();
    if(reponseText.isEmpty())
    {
        mTempResponse.setText(newText, false);
    }
    else
    {
        mTempResponse.setText(reponseText, false);
        mTempResponse.moveCaretToEnd();
    }
}

Application::CoAnalyzer::Chat::QueryEditor::QueryEditor(History const& history)
: mHistory(history)
{
    addListener(this);
}

bool Application::CoAnalyzer::Chat::QueryEditor::keyPressed(juce::KeyPress const& key)
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

void Application::CoAnalyzer::Chat::QueryEditor::resetHistoryIndex()
{
    mHistoryIndex = -1;
}

void Application::CoAnalyzer::Chat::QueryEditor::textEditorTextChanged(juce::TextEditor&)
{
    if(std::exchange(mShouldChange, true))
    {
        mSavedText = getText();
    }
}

ANALYSE_FILE_END
