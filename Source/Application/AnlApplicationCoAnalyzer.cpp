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
        auto const query = mQueryEditor.getText().trim();
        if(query.isNotEmpty())
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        auto const query = mQueryEditor.getText().trim();
        mSendButton.setEnabled(mIsInitialized.load() && (mRequestFuture.valid() || query.isNotEmpty()));
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
        auto response = std::get<1>(result);
        auto const reponseValidation = validateResponse(response, std::get<2>(result));
        if(reponseValidation.failed())
        {
            auto const errorMessage = reponseValidation.getErrorMessage();
            mHistory.push_back(std::make_tuple(MessageType::error, errorMessage));
            mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", errorMessage), juce::dontSendNotification);
        }
        else
        {
            auto const parts = parseResponse(response);
            MiscWeakAssert(!parts.first.isNotEmpty());
            mHistory.push_back(std::make_tuple(MessageType::assistant, parts.first));
            if(parts.second != nullptr)
            {
                auto& documentAccessor = Instance::get().getDocumentAccessor();
                Instance::get().getDocumentDirector().startAction();
                bool hasModification = false;
                for(auto* trackElement : parts.second->getChildWithTagNameIterator("tracks"))
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
                                            return std::make_tuple(juce::Result::ok(), std::string{});
                                        }

                                        MiscWeakAssert(initializeResult.wasOk());
                                        if(initializeResult.failed())
                                        {
                                            return std::make_tuple(std::move(initializeResult), std::string{});
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
                                                return std::make_tuple(std::move(stateResult), std::string{});
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
                                                return std::make_tuple(std::move(systemMessageResult), std::string{});
                                            }
                                            mChat.saveState(state);
                                        }

                                        if(mShouldQuit.load())
                                        {
                                            return std::make_tuple(juce::Result::ok(), std::string{});
                                        }

                                        chrono.start();
                                        auto userQueryResult = mChat.sendUserQuery("Introduce yourself in one sentence.", nullptr);
                                        chrono.stop("Introduction generation ended");
                                        mIsInitialized.store(std::get<0_z>(userQueryResult).wasOk());
                                        return userQueryResult;
                                    }();
                                    triggerAsyncUpdate();
                                    return std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), juce::String{});
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
    auto const identifier = *std::get<1_z>(items).cbegin();
    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
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
    mRequestFuture = std::async(std::launch::async, [this, query, identifier, xmld = std::move(xml)]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Request");
                                    static juce::String const dataTemplate = juce::CharPointer_UTF8(AnlCoAnalyzerData::ContextTemplate_md);
                                    auto const contextData = dataTemplate.replace("FULL_XML_CONTEXT", xmld->toString());
                                    auto perform = [this, &contextData, &identifier](auto const& text)
                                    {
                                        Chrono chrono{"CoAnalyzer"};
                                        chrono.start();
                                        auto contextResult = mChat.injectContext(contextData);
                                        chrono.stop("Context injection ended");
                                        if(contextResult.failed())
                                        {
                                            return std::make_tuple(std::move(contextResult), std::string{});
                                        }
                                        chrono.start();
                                        auto queryResult = mChat.sendUserQuery(text, [identifier](std::string const& reponse)
                                                                               {
                                                                                   return validateResponse(reponse, identifier).wasOk();
                                                                               });
                                        chrono.stop("Query processing ended");
                                        return queryResult;
                                    };
                                    auto result = perform(query);
                                    if(std::get<0>(result).failed() || mShouldQuit.load())
                                    {
                                        triggerAsyncUpdate();
                                        return std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), identifier);
                                    }
                                    auto const reponseValidation = validateResponse(std::get<1>(result), identifier);
                                    if(!reponseValidation.failed())
                                    {
                                        triggerAsyncUpdate();
                                        return std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), identifier);
                                    }
                                    mChat.clearTemporaryResponse();
                                    result = perform(reponseValidation.getErrorMessage() + " Please correct the previous response.");
                                    triggerAsyncUpdate();
                                    return std::make_tuple(std::move(std::get<0>(result)), std::move(std::get<1>(result)), identifier);
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
        mHistoryEditor.scrollDown();
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

std::pair<juce::String, std::unique_ptr<juce::XmlElement>> Application::CoAnalyzer::Chat::parseResponse(std::string const& response)
{
    auto copy = response;
    // Split the text of the response in the form <response>text</response><xml>text</xml> into two strings
    auto const split = [&](const char* start, const char* end, bool include) -> std::string
    {
        auto startIt = copy.find(start);
        if(startIt != std::string::npos)
        {
            auto const startLength = std::strlen(start);
            auto endIt = copy.find(end, startIt + startLength);
            if(endIt != std::string::npos)
            {
                auto const endLength = std::strlen(end);
                auto const extractStart = include ? startIt : startIt + startLength;
                auto const extractEnd = include ? endIt + endLength : endIt;
                auto strResult = copy.substr(extractStart, extractEnd - extractStart);
                auto const eraseStart = startIt;
                auto const eraseEnd = endIt + endLength;
                copy.erase(eraseStart, eraseEnd - eraseStart);
                return strResult;
            }
        }
        return {};
    };
    auto const xml = split("<xml>", "</xml>", true);
    auto const message = split("<response>", "</response>", false);
    return std::make_pair(juce::String(message), juce::parseXML(juce::String(xml)));
}

juce::Result Application::CoAnalyzer::Chat::validateResponse(std::string const& response, juce::String const& identifier)
{
    // Check if response contains the previous context (## CONTEXT), which indicates an error in generation
    {
        auto const contextPos = response.rfind("CONTEXT");
        if(contextPos != std::string::npos)
        {
            return juce::Result::fail("The response must not contain the context.");
        }
    }
    // Count how many times the last responseWindowSize are present in the response
    {
        static constexpr size_t responseWindowSize = 200; // only check for repetition if piece is at least this long
        static constexpr size_t maxRepetition = 10;       // number of times a string can repeat before aborting
        if(response.size() > responseWindowSize)
        {
            auto const startPos = response.size() - responseWindowSize;
            auto occurencePos = response.rfind(response.substr(startPos, responseWindowSize), startPos - 1);
            if(occurencePos != std::string::npos && occurencePos > 0)
            {
                auto const textSequence = response.substr(occurencePos, response.size() - occurencePos);
                occurencePos = response.rfind(textSequence, occurencePos - 1);
                auto textSequenceCount = 1_z;
                while(occurencePos != std::string::npos && occurencePos > 0)
                {
                    ++textSequenceCount;
                    if(textSequenceCount > maxRepetition)
                    {
                        return juce::Result::fail("The response contains too many repetitions.");
                    }
                    occurencePos = response.rfind(textSequence, occurencePos - 1);
                }
            }
        }
    }
    // Check if the format is valid
    {
        auto const responseStartPos = response.find("<response>");
        auto const responseEndPos = response.find("</response>");
        if(responseEndPos != std::string::npos && (responseStartPos == std::string::npos || responseEndPos < responseStartPos))
        {
            return juce::Result::fail("The response format is corrupted: missing <response> tags.");
        }
        auto const xmlStartPos = response.find("<xml>");
        auto const xmlEndPos = response.find("</xml>");
        if(xmlEndPos != std::string::npos && (xmlStartPos == std::string::npos || xmlEndPos < xmlStartPos))
        {
            return juce::Result::fail("The xml format is corrupted: missing <xml> tags.");
        }
        if(xmlEndPos == std::string::npos)
        {
            return juce::Result::ok();
        }
        auto const xmlLength = xmlEndPos - xmlStartPos + std::strlen("</xml>");
        auto const xmlContent = juce::String(response.substr(xmlStartPos, xmlLength)).trim();
        if(xmlContent.isEmpty())
        {
            return juce::Result::ok();
        }
        juce::XmlDocument document{xmlContent};
        auto const element = document.getDocumentElement();
        auto const lasError = document.getLastParseError();
        if(lasError.isNotEmpty())
        {
            return juce::Result::fail("The xml format is corrupted: " + lasError + ".");
        }
        if(element == nullptr)
        {
            return juce::Result::fail("The xml format is corrupted: failed to parse.");
        }
        if(element->getNumChildElements() == 0 && element->getNumAttributes() == 0)
        {
            return juce::Result::ok();
        }
        auto* track = element->getChildByName("tracks");
        if(track == nullptr)
        {
            return juce::Result::fail("The xml format is corrupted: missing <tracks> element.");
        }
        if(!track->hasAttribute("identifier"))
        {
            return juce::Result::fail("The xml format is corrupted: missing identifier attribute in <tracks> element.");
        }
        if(track->getStringAttribute("identifier") != identifier)
        {
            return juce::Result::fail("The xml format is corrupted: identifier attribute doesn't match the selected track.");
        }
    }
    return juce::Result::ok();
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
