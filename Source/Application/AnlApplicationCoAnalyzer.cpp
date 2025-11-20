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
, mSendButton(juce::ImageCache::getFromMemory(AnlIconsData::send_png, AnlIconsData::send_pngSize))
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
        if(canSendQuery())
        {
            sendUserQuery();
        }
    };
    mQueryEditor.onTextChange = [this]()
    {
        mSendButton.setEnabled(canSendQuery());
    };

    mSendButton.setTooltip(juce::translate("Send Query"));
    mSendButton.onClick = [this]()
    {
        sendUserQuery();
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
    if(mRequestFuture.valid())
    {
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
            updateHistory();
        }
    }
    auto const enabled = canSendQuery();
    mQueryEditor.setEnabled(!mRequestFuture.valid());
    mSendButton.setEnabled(enabled);
    mQueryEditor.setTextToShowWhenEmpty(juce::translate("Enter your query in natural language..."), findColour(juce::TextEditor::ColourIds::textColourId).withAlpha(0.5f));
}

bool Application::CoAnalyzer::Chat::canSendQuery() const
{
    if(mRequestFuture.valid())
    {
        return false;
    }
    auto const query = mQueryEditor.getText();
    if(query.isEmpty())
    {
        return false;
    }
    return true;
}

void Application::CoAnalyzer::Chat::initializeSystem()
{
    if(mRequestFuture.valid())
    {
        mShouldQuit.store(true);
        cancelPendingUpdate();
        handleAsyncUpdate();
    }

    mIsInitialized = false;

    mIsInitialized.store(false);
    mHistory.clear();
    updateHistory();
    mQueryEditor.setEnabled(!mRequestFuture.valid());
    mSendButton.setEnabled(false);
    mStatusLabel.setText(juce::translate("Initializing..."), juce::dontSendNotification);
    mQueryEditor.setTextToShowWhenEmpty(juce::translate("Initializing..."), juce::Colours::grey);

    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, model = mAccessor.getAttr<AttrType::model>()]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Initialize");
                                    auto const initialized = mChat.initialize(model);
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }

                                    MiscWeakAssert(initialized.ok());
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }
                                    MiscDebug("Application::CoAnalyzer::Chat", "Load context from /Users/guillot/Git/Partiels/README.md");
                                    mChat.addContext(juce::File("/Users/guillot/Git/Partiels/README.md").loadFileAsString());
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }
                                    MiscDebug("Application::CoAnalyzer::Chat", "Load context from /Users/guillot/Git/Partiels/build/Partiels-Manual.md");
                                    mChat.addContext(juce::File("/Users/guillot/Git/Partiels/build/Partiels-Manual.md").loadFileAsString());
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }
                                    MiscDebug("Application::CoAnalyzer::Chat", "Load context from /Users/guillot/Git/Partiels/BinaryData/Resource/FactoryTemplate.ptldoc");
                                    mChat.addContext(juce::File("/Users/guillot/Git/Partiels/BinaryData/Resource/FactoryTemplate.ptldoc").loadFileAsString());
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }
                                    mChat.loadState({"/Users/guillot/Git/Partiels-training/raethehacker_Llama-3.1-8B-Instruct-Q2_K-GGUF_llama-3.1-8b-instruct-q2_k-cache.bin"});
                                    if(mShouldQuit.load())
                                    {
                                        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
                                    }
                                    auto result = mChat.generate(Role::system, "Can you introduce you?");
                                    mIsInitialized.store(initialized.ok());
                                    triggerAsyncUpdate();
                                    return result;
                                });
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
    auto xmlElement = documentAccessor.toXml("Document");
    if(xmlElement == nullptr)
    {
        mStatusLabel.setText(juce::translate("Error: Could not export document"), juce::dontSendNotification);
        return;
    }

    mQueryEditor.setText({}, juce::sendNotificationSync);
    mStatusLabel.setText(juce::translate("Sending query..."), juce::dontSendNotification);

    mHistory.push_back(std::make_tuple(Role::user, query));
    mShouldQuit.store(false);
    MiscWeakAssert(!mRequestFuture.valid());
    mRequestFuture = std::async(std::launch::async, [this, query]()
                                {
                                    juce::Thread::setCurrentThreadName("CoAnalyzer::Chat::Request");
                                    auto result = mChat.generate(Role::user, query);
                                    triggerAsyncUpdate();
                                    return result;
                                });

    updateHistory();
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
        mHistoryEditor.insertTextAtCaret(roleStr + ": " + request + "\n");
    }
}

ANALYSE_FILE_END
