#include "AnlApplicationCoAnalyzer.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlResourceData.h>

ANALYSE_FILE_BEGIN

Application::CoAnalyzer::SettingsContent::SettingsContent()
: mModel(juce::translate("Model"), juce::translate("The model used by the Co-Analyzer"), "", {}, [this](size_t index)
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
    setContent(juce::translate("Co-Analyzer Settings"), &mContent);
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
        mStatusLabel.setText(juce::translate("Error: ERROR").replace("ERROR", std::get<0>(result).getErrorMessage()), juce::dontSendNotification);
    }
    else
    {
        mStatusLabel.setText(juce::translate("Please enter a query"), juce::dontSendNotification);
        if(std::get<1>(result).isNotEmpty())
        {
            mHistory.push_back(std::make_tuple(Role::assistant, std::get<1>(result)));
        }
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
                Instance::get().getDocumentDirector().endAction(ActionState::newTransaction, juce::translate("Co-Analyzer update document"));
            }
        }
        updateHistory();
    }
    mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
    mSendButton.setToggleState(mIsInitialized.load(), juce::NotificationType::dontSendNotification);
    mSendButton.setEnabled(mIsInitialized.load() && mQueryEditor.getText().isNotEmpty());
    mSendButton.setTooltip(juce::translate("Send Query"));
}

Application::CoAnalyzer::Chat::Results Application::CoAnalyzer::Chat::performSystemInitialization(Llama::Chat& chat, juce::File const& model, std::atomic<bool> const& shouldQuit)
{
    auto const initialized = chat.initialize(model);
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
    MiscDebug("Application::CoAnalyzer::Chat", "Load state");
    chat.loadState(model.withFileExtension("bin"));
    if(shouldQuit.load())
    {
        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
    }

    // Inject documentation resources as raw context (faster than addContext for large files)
    // These files should be embedded in BinaryData or stored in application resources
    for(auto i = 1; i <= 4; ++i)
    {
        MiscDebug("Application::CoAnalyzer::Chat", "Injecting context from Resource " + juce::String(i));

        // TODO: Replace hardcoded path with embedded resource or application data directory
        // Example: auto content = String::createStringFromData(BinaryData::Resource_1_md, BinaryData::Resource_1_mdSize);
        auto const resourcePath = juce::File("/Users/guillot/Git/Partiels-training/data/Resource_" + juce::String(i) + ".md");
        if(!resourcePath.existsAsFile())
        {
            MiscDebug("Application::CoAnalyzer::Chat", "Warning: Resource file not found: " + resourcePath.getFullPathName());
            continue;
        }

        auto const content = resourcePath.loadFileAsString();
        if(content.isEmpty())
        {
            MiscDebug("Application::CoAnalyzer::Chat", "Warning: Resource file is empty: " + resourcePath.getFullPathName());
            continue;
        }

        // Use injectRawContext instead of addContext - much faster for large documents
        auto result = chat.injectRawContext(content);
        if(result.failed())
        {
            MiscDebug("Application::CoAnalyzer::Chat", "Failed to inject resource: " + result.getErrorMessage());
            return std::make_tuple(result, juce::String{}, juce::String{});
        }

        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
    }

    // Initialize the installed plugin list as context
    {
        MiscDebug("Application::CoAnalyzer::Chat", "Load context from plugin list");
        auto const plugins = Instance::get().getPluginListScanner().getPlugins(48000.0);
        juce::XmlElement xml("plugins");
        for(auto const& plugin : std::get<0>(plugins))
        {
            auto pluginXml = std::make_unique<juce::XmlElement>("installed_plugin");
            MiscWeakAssert(pluginXml != nullptr && "Cannot allocate plugin XML element!");
            if(pluginXml != nullptr)
            {
                XmlParser::toXml(*pluginXml, "key", plugin.first);
                // XmlParser::toXml(*pluginXml, "description", plugin.second);
                xml.addChildElement(pluginXml.release());
            }
        }
        // Use injectRawContext for structured data - faster than addContext
        chat.injectRawContext(xml.toString());
        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
    }
    // Initialize the web references of the plugins as context
    {
        MiscDebug("Application::CoAnalyzer::Chat", "Load context from web references");
        auto pluginXml = std::make_unique<juce::XmlElement>("plugin_web_references");
        XmlParser::toXml(*pluginXml.get(), "webReferences", Instance::get().getPluginListAccessor().getAttr<PluginList::AttrType::webReferences>());
        // Use injectRawContext for structured data - faster than addContext
        chat.injectRawContext(pluginXml->toString());
        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
    }

    // Generate the first response to confirm initialization
    MiscDebug("Application::CoAnalyzer::Chat", "Generate the first reponse");
    return chat.generate(Role::system, "Please, introduce yourself briefly.");
}

void Application::CoAnalyzer::Chat::initializeSystem()
{
    stopUserQuery();
    mIsInitialized = false;

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
    auto const query = mQueryEditor.getText();
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
                                 .withTitle(juce::translate("Co-Analyzer Model Not Set"))
                                 .withMessage(juce::translate("The Co-Analyzer model is not set. Do you want to open the Co-Analyzer settings panel to set it now?"))
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
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto xml = documentAccessor.toXml("Document");
    if(xml == nullptr)
    {
        mStatusLabel.setText(juce::translate("Error: Could not export document"), juce::dontSendNotification);
        return;
    }

    mQueryEditor.setText({}, juce::sendNotificationSync);
    mStatusLabel.setText(juce::translate("Processing query"), juce::dontSendNotification);
    mSendButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    mSendButton.setTooltip(juce::translate("Stop Query"));

    mHistory.push_back(std::make_tuple(Role::user, query));
    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query, xmld = std::move(xml)]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Request");
                                    auto const data = "Here is the content of the current document: ```xml" + xmld->toString() + "```";
                                    // Use injectRawContext for large document XML - faster than addContext
                                    auto contextResult = mChat.injectRawContext(data);
                                    if(contextResult.failed())
                                    {
                                        triggerAsyncUpdate();
                                        return std::make_tuple(std::move(contextResult), juce::String{}, juce::String{});
                                    }
                                    auto result = mChat.generate(Role::user, query);
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
            case Role::user:
                roleStr = juce::translate("User");
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, userColour);
                break;
            case Role::assistant:
                roleStr = juce::translate("Assistant");
                mHistoryEditor.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colours::green);
                break;
            case Role::system:
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
