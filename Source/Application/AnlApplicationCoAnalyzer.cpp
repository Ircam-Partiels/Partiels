#include "AnlApplicationCoAnalyzer.h"
#include "../Document/AnlDocumentSelection.h"
#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlResourceData.h>

ANALYSE_FILE_BEGIN

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
{
    mInstalledModels = []()
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
        return models;
    }();

    addAndMakeVisible(mModel);
    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
    mModel.entry.setTextWhenNoChoicesAvailable(juce::translate("No model installed"));
    for(auto const& model : mInstalledModels)
    {
        mModel.entry.addItem(model.getFileName(), static_cast<int>(mModel.entry.getNumItems()) + 1);
    }
    mListener.onAttrChanged = [this](Accessor const& accessor, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::model:
            {
                auto const model = accessor.getAttr<AttrType::model>();
                auto const it = std::find(mInstalledModels.cbegin(), mInstalledModels.cend(), model);
                if(it != mInstalledModels.cend() && it->existsAsFile())
                {
                    mModel.entry.setSelectedId(static_cast<int>(std::distance(mInstalledModels.cbegin(), it)) + 1, juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
                }
                break;
            }
        }
    };
    setSize(300, 200);
    auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
    accessor.addListener(mListener, NotificationType::synchronous);
}

Application::CoAnalyzer::SettingsContent::~SettingsContent()
{
    auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::coAnalyzer>();
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
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

Application::CoAnalyzer::SettingsPanel::SettingsPanel()
{
    setContent(juce::translate("Neuralyzer Settings"), &mContent);
}

Application::CoAnalyzer::SettingsPanel::~SettingsPanel()
{
    setContent("", nullptr);
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

    addAndMakeVisible(mHistoryEditor);
    addAndMakeVisible(mSeparator2);
    addAndMakeVisible(mQueryEditor);
    addAndMakeVisible(mSendButton);
    addAndMakeVisible(mSeparator1);
    addAndMakeVisible(mStatusLabel);

    mListener.onAttrChanged = [this](Accessor const&, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::model:
            {
                initializeSystem();
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::CoAnalyzer::Chat::~Chat()
{
    mAccessor.removeListener(mListener);
    mShouldQuit.store(true);
    if(mRequestFuture.valid())
    {
        mRequestFuture.wait();
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
    mSeparator2.setBounds(bounds.removeFromBottom(1));
    mQueryEditor.setBounds(bounds.removeFromBottom(PropertyComponentBase::defaultHeight * 7).reduced(4, 0));
    mSendButton.setBounds(mQueryEditor.getBounds().removeFromRight(18).removeFromBottom(18));
    mSeparator1.setBounds(bounds.removeFromBottom(1));
    mHistoryEditor.setBounds(bounds.reduced(4, 0));
}

void Application::CoAnalyzer::Chat::colourChanged()
{
    updateHistory();
}

void Application::CoAnalyzer::Chat::parentHierarchyChanged()
{
    colourChanged();
}

void Application::CoAnalyzer::Chat::handleAsyncUpdate()
{
    stopTimer();

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
            auto const xml = juce::parseXML(std::get<2>(result));
            if(xml != nullptr)
            {
                MiscDebug("Application::CoAnalyzer::Chat", xml->toString());
                Instance::get().getDocumentDirector().startAction();
                for(auto* trackElement : xml->getChildWithTagNameIterator("tracks"))
                {
                    JUCE_COMPILER_WARNING("Ensure the idenfitifier corresponds to the one used")
                    if(trackElement != nullptr && trackElement->hasAttribute("identifier"))
                    {
                        auto const identifier = trackElement->getStringAttribute("identifier");
                        if(Document::Tools::hasTrackAcsr(documentAccessor, identifier))
                        {
                            auto& srcAcsr = Document::Tools::getTrackAcsr(documentAccessor, identifier);
                            auto const layout = srcAcsr.getAttr<Track::AttrType::channelsLayout>();
                            srcAcsr.fromXml(*trackElement, "tracks", NotificationType::synchronous);
                            srcAcsr.setAttr<Track::AttrType::channelsLayout>(layout, NotificationType::synchronous);
                        }
                    }
                }
                Instance::get().getDocumentDirector().endAction(ActionState::newTransaction, juce::translate("Neuralyzer update document"));
            }
        }
    }
    updateHistory();
    mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    mSendButton.setToggleState(mIsInitialized.load(), juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(mIsInitialized.load() && mQueryEditor.getText().isNotEmpty());
    mSendButton.setTooltip(juce::translate("Send Query"));
}

Application::CoAnalyzer::Chat::Results Application::CoAnalyzer::Chat::performSystemInitialization(Llama::Chat& chat, juce::File const& model, std::atomic<bool> const& shouldQuit)
{
    MiscDebug("Application::CoAnalyzer::Chat", "Initializing Neuralyzer chat system...");
    Chrono chrono{"CoAnalyzer"};

    chrono.start();
    auto const initialized = chat.initialize(model);
    chrono.stop("Initialization ended");

    if(shouldQuit.load())
    {
        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
    }

    MiscWeakAssert(initialized.wasOk());
    if(initialized.failed())
    {
        return std::make_tuple(initialized, juce::String{}, juce::String{});
    }

    // Load the saved state of the model
    auto const state = model.withFileExtension("bin");
    if(state.existsAsFile())
    {
        chrono.start();
        auto const result = chat.loadState(state);
        chrono.stop("State load ended");
        if(result.failed())
        {
            MiscDebug("Application::CoAnalyzer::Chat", "Failed to inject state: " + result.getErrorMessage());
            return std::make_tuple(result, juce::String{}, juce::String{});
        }
        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
    }
    else
    {
        chrono.start();
        JUCE_COMPILER_WARNING("Replace with data embedded in BinaryData or application resources")
        auto const instructionText = juce::File("/Users/guillot/Git/Partiels/BinaryData/CoAnalyzer/Instructions_0.md").loadFileAsString();
        auto const result = chat.addSystemMessage(instructionText);
        chrono.stop("State generation ended");
        if(result.failed())
        {
            MiscDebug("Application::CoAnalyzer::Chat", "Failed to initialize state: " + result.getErrorMessage());
            return std::make_tuple(result, juce::String{}, juce::String{});
        }
        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
        chat.saveState(state);
    }

    chrono.start();
    auto result = chat.sendUserQuery("Introduce yourself in one sentence.");
    chrono.stop("Introduction generation ended");
    return std::make_tuple(juce::Result::ok(), std::get<1>(result), std::get<2>(result));
}

void Application::CoAnalyzer::Chat::initializeSystem()
{
    stopUserQuery();

    mIsInitialized.store(false);
    mHistory.clear();
    updateHistory();
    mSendButton.setEnabled(false);
    mStatusLabel.setText(juce::translate("Initializing..."), juce::dontSendNotification);

    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());

    mRequestFuture = std::async(std::launch::async, [this, model = mAccessor.getAttr<AttrType::model>()]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Initialize");
                                    auto result = performSystemInitialization(mChat, model, mShouldQuit);
                                    mIsInitialized.store(std::get<0_z>(result).wasOk());
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
                                         if(auto* window = Instance::get().getWindow())
                                         {
                                             window->getInterface().showCoAnalyzerSettingsPanel();
                                         }
                                     });
        return;
    };

    // Get current document XML
    auto xml = []()
    {
        auto& documentAcsr = Instance::get().getDocumentAccessor();
        auto const items = Document::Selection::getItems(documentAcsr);
        if(!std::get<0_z>(items).empty())
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, *std::get<0_z>(items).cbegin());
            auto element = groupAcsr.toXml("groups");
            return std::make_tuple("group", std::move(element));
        }
        if(!std::get<1_z>(items).empty())
        {
            auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, *std::get<1_z>(items).cbegin());
            auto element = trackAcsr.toXml("tracks");
            element->getChildByName("description")->getChildByName("output")->deleteAllChildElementsWithTagName("binNames");
            return std::make_tuple("track", std::move(element));
        }
        auto element = documentAcsr.toXml("Document");
        for(auto* child = element->getChildByName("groups"); child != nullptr; child = child->getNextElementWithTagName("groups"))
        {
            child->deleteAllChildElements();
        }
        for(auto* child = element->getChildByName("tracks"); child != nullptr; child = child->getNextElementWithTagName("tracks"))
        {
            child->deleteAllChildElements();
        }
        return std::make_tuple("document", std::move(element));
    }();
    if(std::get<1>(xml) == nullptr)
    {
        mStatusLabel.setText(juce::translate("Error: Could not export selection as XML"), juce::dontSendNotification);
        return;
    }

    mQueryEditor.setText({}, juce::sendNotificationSync);
    mStatusLabel.setText(juce::translate("Processing query"), juce::dontSendNotification);
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mHistory.push_back(std::make_tuple(MessageType::user, query));
    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query, xmld = std::move(xml)]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Request");
                                    auto const data = juce::String("Here is the content of the current ") + std::get<0>(xmld) + ": ```xml" + std::get<1>(xmld)->toString() + "```";
                                    auto contextResult = mChat.injectContext(data);
                                    if(contextResult.failed())
                                    {
                                        triggerAsyncUpdate();
                                        return std::make_tuple(std::move(contextResult), juce::String{}, juce::String{});
                                    }
                                    auto result = mChat.sendUserQuery(query);
                                    triggerAsyncUpdate();
                                    return result;
                                });
    startTimer(250);
    updateHistory();
}

void Application::CoAnalyzer::Chat::stopUserQuery()
{
    if(mRequestFuture.valid())
    {
        mShouldQuit.store(true);
        cancelPendingUpdate();
        handleAsyncUpdate();
    }
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
}

ANALYSE_FILE_END
