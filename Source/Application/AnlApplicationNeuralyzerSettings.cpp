#include "AnlApplicationNeuralyzerSettings.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

namespace
{
    static juce::String toDisplayString(juce::String const& modelId)
    {
        auto const names = modelId.fromLastOccurrenceOf("/", false, false).upToLastOccurrenceOf(".gguf", false, false).replace("-", " ").trim();
        auto words = juce::StringArray::fromTokens(names, " ", "");
        words.trim();
        words.removeEmptyStrings();
        return words.joinIntoString(" ");
    }
} // namespace

Application::Neuralyzer::SettingsContent::SettingsContent(Accessor& accessor)
: mAccessor(accessor)
, mBackend(juce::translate("Backend"), juce::translate("The backend used to run the Neuralyzer"), "", {juce::translate("Local"), juce::translate("Remote")}, [&](size_t index)
           {
               auto const backend = magic_enum::enum_cast<AgentBackend>(static_cast<int>(index)).value_or(AgentBackend::local);
               mAccessor.setAttr<AttrType::agentBackend>(backend, NotificationType::synchronous);
           })
, mRemoteUrl(juce::translate("Server URL"), juce::translate("The URL of the remote server (scheme and host, e.g. http://localhost:1234)"), [&](juce::String text)
             {
                 auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                 modelInfo.modelUrl = juce::URL(text);
                 mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
             })
, mModel(juce::translate("Model"), juce::translate("The model used by the Neuralyzer"), "", {}, nullptr)
, mContextSize(juce::translate("Context Size"), juce::translate("The context size"), "", {static_cast<double>(minContextSize), static_cast<double>(maxContextSize)}, 1.0, [&](double value)
               {
                   auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                   auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
                   modelInfo.contextSize = mContextSize.entry.isOptional() ? std::optional<int32_t>{} : std::make_optional(static_cast<int32_t>(value));
                   effectiveState.contextSize = modelInfo.contextSize;
                   mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
                   mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
               })
, mBatchSize(juce::translate("Batch Size"), juce::translate("The batch size"), "", {static_cast<double>(minBatchSize), static_cast<double>(maxBatchSize)}, 1.0, [&](double value)
             {
                 auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                 auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
                 modelInfo.batchSize = mBatchSize.entry.isOptional() ? std::optional<int32_t>{} : std::make_optional(static_cast<int32_t>(value));
                 effectiveState.batchSize = modelInfo.batchSize;
                 mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
                 mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
             })
, mMinP(juce::translate("Min. Probability"), juce::translate("Minimum probability for sampling"), "", {0.0, 1.0}, 0.01, [&](double value)
        {
            auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
            auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
            modelInfo.minP = mMinP.entry.isOptional() ? std::optional<float>{} : std::make_optional(static_cast<float>(value));
            effectiveState.minP = modelInfo.minP;
            mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
            mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
        })
, mTemperature(juce::translate("Temperature"), juce::translate("Temperature for sampling"), "", {0.0, 2.0}, 0.01, [&](double value)
               {
                   auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                   auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
                   modelInfo.temperature = mTemperature.entry.isOptional() ? std::optional<float>{} : std::make_optional(static_cast<float>(value));
                   effectiveState.temperature = modelInfo.temperature;
                   mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
                   mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
               })
, mTopP(juce::translate("Top P"), juce::translate("Top-p (nucleus) sampling probability"), "", {0.0, 1.0}, 0.01, [&](double value)
        {
            auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
            auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
            modelInfo.topP = mTopP.entry.isOptional() ? std::optional<float>{} : std::make_optional(static_cast<float>(value));
            effectiveState.topP = modelInfo.topP;
            mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
            mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
        })
, mTopK(juce::translate("Top K"), juce::translate("Top-k sampling (number of top tokens to consider)"), "", {static_cast<double>(minTopK), static_cast<double>(maxTopK)}, 1.0, [&](double value)
        {
            auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
            auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
            modelInfo.topK = mTopK.entry.isOptional() ? std::optional<int32_t>{} : std::make_optional(static_cast<int32_t>(value));
            effectiveState.topK = modelInfo.topK;
            mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
            mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
        })
, mPresencePenalty(juce::translate("Presence Penalty"), juce::translate("Penalty for token presence in previous text"), "", {-2.0, 2.0}, 0.01, [&](double value)
                   {
                       auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                       auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
                       modelInfo.presencePenalty = mPresencePenalty.entry.isOptional() ? std::optional<float>{} : std::make_optional(static_cast<float>(value));
                       effectiveState.presencePenalty = modelInfo.presencePenalty;
                       mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
                       mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
                   })
, mRepetitionPenalty(juce::translate("Repetition Penalty"), juce::translate("Penalty for repeating tokens"), "", {1.0, 2.0}, 0.01, [&](double value)
                     {
                         auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                         auto effectiveState = mAccessor.getAttr<AttrType::effectiveState>();
                         modelInfo.repetitionPenalty = mRepetitionPenalty.entry.isOptional() ? std::optional<float>{} : std::make_optional(static_cast<float>(value));
                         effectiveState.repetitionPenalty = modelInfo.repetitionPenalty;
                         mAccessor.setAttr<AttrType::effectiveState>(effectiveState, NotificationType::synchronous);
                         mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
                     })
, mModelsDirectory(juce::translate("Models Directory"), juce::translate("Reveal the directory where models are stored"), []()
                   {
                       AgentLocal::getDefaultModelDirectory().revealToUser();
                   })
, mRagDownload(juce::translate("Download RAG models"), juce::translate("Download the RAG embedding and reranker models automatically"), []()
               {
                   Rag::downloadRagModelsIfNecessary();
               })
{
    addAndMakeVisible(mBackend);
    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
    mModel.entry.setTextWhenNoChoicesAvailable(juce::translate("No model installed"));
    addAndMakeVisible(mBackendSeparator);
    mBackendSeparator.setSize(1, 1);

    addAndMakeVisible(mRemoteUrl);
    addAndMakeVisible(mModel);
    addAndMakeVisible(mContextSize);
    mContextSize.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mBatchSize);
    mBatchSize.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mMinP);
    mMinP.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mTemperature);
    mTemperature.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mTopP);
    mTopP.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mTopK);
    mTopK.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mPresencePenalty);
    mPresencePenalty.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mRepetitionPenalty);
    mRepetitionPenalty.entry.setOptionalSupported(true, juce::translate("Default"));
    addAndMakeVisible(mDownloadSeparator);
    mDownloadSeparator.setSize(1, 1);
    addAndMakeVisible(mModelsDirectory);
    addAndMakeVisible(mRagDownload);

    mModel.entry.onShowPopup = [this]()
    {
        showModelMenu();
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelInfo:
            case AttrType::effectiveState:
                break;
            case AttrType::agentBackend:
            {
                auto const backend = acsr.getAttr<AttrType::agentBackend>();
                auto const index = magic_enum::enum_index(backend).value_or(0_z);
                mBackend.entry.setSelectedItemIndex(static_cast<int>(index), juce::NotificationType::dontSendNotification);
                mRemoteUrl.setVisible(backend == AgentBackend::remote);
                mContextSize.setVisible(backend == AgentBackend::local);
                mBatchSize.setVisible(backend == AgentBackend::local);
                mMinP.setVisible(backend == AgentBackend::local);
                mTemperature.setVisible(backend == AgentBackend::local);
                mTopP.setVisible(backend == AgentBackend::local);
                mTopK.setVisible(backend == AgentBackend::local);
                mPresencePenalty.setVisible(backend == AgentBackend::local);
                mRepetitionPenalty.setVisible(backend == AgentBackend::local);
                mDownloadSeparator.setVisible(backend == AgentBackend::local);
                mModelsDirectory.setVisible(backend == AgentBackend::local);
                resized();
                break;
            }
        }

        auto const setEntryValue = [&](auto& entry, auto const& value, auto const& defaultValue)
        {
            if(value.has_value())
            {
                entry.setValue(static_cast<double>(value.value()), juce::NotificationType::dontSendNotification);
            }
            else if(defaultValue.has_value())
            {
                if constexpr(std::is_same_v<std::decay_t<decltype(defaultValue)>, std::optional<float>>)
                {
                    entry.setText(juce::translate("VALUE (Default)").replace("VALUE", juce::String(defaultValue.value(), 2)), juce::NotificationType::dontSendNotification);
                }
                else
                {
                    entry.setText(juce::translate("VALUE (Default)").replace("VALUE", juce::String(defaultValue.value())), juce::NotificationType::dontSendNotification);
                }
            }
            else
            {
                entry.setText(juce::translate("Default"), juce::NotificationType::dontSendNotification);
            }
        };
        auto const modelInfo = acsr.getAttr<AttrType::modelInfo>();
        auto const defaultState = acsr.getAttr<AttrType::effectiveState>();
        mRemoteUrl.entry.setText(modelInfo.modelUrl.toString(true), juce::NotificationType::dontSendNotification);
        setEntryValue(mContextSize.entry, modelInfo.contextSize, defaultState.contextSize);
        setEntryValue(mBatchSize.entry, modelInfo.batchSize, defaultState.batchSize);
        setEntryValue(mMinP.entry, modelInfo.minP, defaultState.minP);
        setEntryValue(mTemperature.entry, modelInfo.temperature, defaultState.temperature);
        setEntryValue(mTopP.entry, modelInfo.topP, defaultState.topP);
        setEntryValue(mTopK.entry, modelInfo.topK, defaultState.topK);
        setEntryValue(mPresencePenalty.entry, modelInfo.presencePenalty, defaultState.presencePenalty);
        setEntryValue(mRepetitionPenalty.entry, modelInfo.repetitionPenalty, defaultState.repetitionPenalty);
        postCommandMessage(0);
    };

    setSize(300, 200);
    Instance::get().getNeuralyzerDownloaderManager().addChangeListener(this);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

void Application::Neuralyzer::SettingsContent::showModelMenu()
{
    juce::PopupMenu menu;
    auto const modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
    auto const backend = mAccessor.getAttr<AttrType::agentBackend>();
    if(backend == AgentBackend::remote)
    {
        auto const remoteModels = AgentRemote::getAvailableModels(modelInfo.modelUrl);
        for(auto const& remoteModel : remoteModels)
        {
            auto const isCurrent = remoteModel == modelInfo.modelId;
            menu.addItem(toDisplayString(remoteModel), true, isCurrent, [=, this]()
                         {
                             mAccessor.setAttr<AttrType::effectiveState>(ModelInfo{}, NotificationType::synchronous);
                             auto currentModelInfo = mAccessor.getAttr<AttrType::modelInfo>();
                             currentModelInfo.modelId = remoteModel;
                             mAccessor.setAttr<AttrType::modelInfo>(currentModelInfo, NotificationType::synchronous);
                         });
        }
    }
    else
    {
        auto localModels = AgentLocal::getAvailableModels();
        auto const currentModel = modelInfo.modelFile;
        for(auto const& defaultModelUrl : AgentLocal::getDefaultModelUrls())
        {
            auto const modelName = juce::String(defaultModelUrl.getFileName());
            auto const displayName = toDisplayString(modelName);
            auto const localModelIt = std::find_if(localModels.begin(), localModels.end(), [&](auto const& installedModel)
                                                   {
                                                       return installedModel.getFileName().toLowerCase() == modelName.toLowerCase();
                                                   });
            if(localModelIt == localModels.end())
            {
                static const auto downloadIndicator = juce::CharPointer_UTF8(" \xf0\x9f\x8c\x90"); // 🌐
                auto const processes = Instance::get().getNeuralyzerDownloaderManager().getProcesses();
                auto const isNotDownloading = std::none_of(processes.cbegin(), processes.cend(), [&](auto const& process)
                                                           {
                                                               return process.get().getOutputFile().getFileName().toLowerCase() == modelName.toLowerCase();
                                                           });
                menu.addItem(displayName + juce::String(downloadIndicator), isNotDownloading, false, [=, this]()
                             {
                                 auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
                                 auto weakRef = juce::WeakReference<juce::Component>(this);
                                 downloader.start(AgentLocal::getDefaultModelDirectory(), defaultModelUrl, [=, this](Downloader::Process const& process)
                                                  {
                                                      if(weakRef.get() == nullptr)
                                                      {
                                                          return;
                                                      }
                                                      showDownloadAlert(process.getOutputFile(), process.getResult());
                                                  });
                             });
            }
            else
            {
                auto const model = *localModelIt;
                menu.addItem(displayName, true, model == currentModel, [=, this]()
                             {
                                 mAccessor.setAttr<AttrType::effectiveState>(ModelInfo{}, NotificationType::synchronous);
                                 mAccessor.setAttr<AttrType::modelInfo>(ModelInfo(model), NotificationType::synchronous);
                             });
                localModels.erase(localModelIt);
            }
        }

        if(menu.getNumItems() > 0 && !localModels.empty())
        {
            menu.addSeparator();
        }
        for(auto const& model : localModels)
        {
            auto const displayName = toDisplayString(model.getFileNameWithoutExtension());
            menu.addItem(displayName, true, model == currentModel, [=, this]()
                         {
                             mAccessor.setAttr<AttrType::effectiveState>(ModelInfo{}, NotificationType::synchronous);
                             mAccessor.setAttr<AttrType::modelInfo>(ModelInfo(model), NotificationType::synchronous);
                         });
        }
    }
    auto& lf = getLookAndFeel();
    menu.setLookAndFeel(&lf);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mModel.entry).withDeletionCheck(*this), [this](int)
                       {
                           mModel.entry.hidePopup();
                       });
}

void Application::Neuralyzer::SettingsContent::showDownloadAlert(juce::File const& file, juce::Result const& result)
{
    if(isShowing())
    {
        return;
    }
    if(result.failed())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                 .withTitle(juce::translate("Failed to download the model NAME").replace("NAME", file.getFileNameWithoutExtension()))
                                 .withMessage(result.getErrorMessage())
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
    else
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Models Downloaded"))
                                 .withMessage(juce::translate("The model NAME has been successfully downloaded. You can now select it in the model list. Would you like to open the Neuralyzer settings panel to select it now?").replace("NAME", file.getFileNameWithoutExtension()))
                                 .withButton(juce::translate("Open"))
                                 .withButton(juce::translate("Close"));
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
}

Application::Neuralyzer::SettingsContent::~SettingsContent()
{
    Instance::get().getNeuralyzerDownloaderManager().removeChangeListener(this);
    mAccessor.removeListener(mListener);
}

void Application::Neuralyzer::SettingsContent::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mBackend);
    setBounds(mBackendSeparator);
    setBounds(mRemoteUrl);
    setBounds(mModel);
    setBounds(mContextSize);
    setBounds(mBatchSize);
    setBounds(mMinP);
    setBounds(mTemperature);
    setBounds(mTopP);
    setBounds(mTopK);
    setBounds(mPresencePenalty);
    setBounds(mRepetitionPenalty);
    setBounds(mDownloadSeparator);
    setBounds(mModelsDirectory);
    setBounds(mRagDownload);
    for(auto const& view : mDownloadViews)
    {
        setBounds(*view.get());
    }
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Neuralyzer::SettingsContent::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    postCommandMessage(0);
}

void Application::Neuralyzer::SettingsContent::handleCommandMessage([[maybe_unused]] int commandId)
{
    if(!isShowing())
    {
        return;
    }

    // Update the list of downloading models
    auto const processes = Instance::get().getNeuralyzerDownloaderManager().getProcesses();
    mDownloadViews.clear();
    mDownloadViews.reserve(processes.size());
    for(auto& process : processes)
    {
        auto view = std::make_unique<Neuralyzer::Downloader::View>(process);
        addAndMakeVisible(view.get());
        mDownloadViews.push_back(std::move(view));
    }

    // Update the model menu and selected model
    auto const backend = mAccessor.getAttr<AttrType::agentBackend>();
    auto const modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
    if(backend == AgentBackend::remote)
    {
        auto const remoteModelIds = AgentRemote::getAvailableModels(modelInfo.modelUrl);
        auto const it = remoteModelIds.find(modelInfo.modelId);
        if(it != remoteModelIds.cend() && !modelInfo.modelId.isEmpty())
        {
            auto const displayName = toDisplayString(*it);
            mModel.entry.setText(displayName, juce::NotificationType::dontSendNotification);
        }
        else if(modelInfo.modelId.isEmpty())
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else if(!modelInfo.modelId.isEmpty())
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Model cannot be found"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
    }
    else
    {
        auto const localModels = AgentLocal::getAvailableModels();
        auto const it = localModels.find(modelInfo.modelFile);
        if(it != localModels.cend() && it->existsAsFile())
        {
            auto const displayName = toDisplayString(it->getFileNameWithoutExtension());
            mModel.entry.setText(displayName, juce::NotificationType::dontSendNotification);
        }
        else if(modelInfo.modelFile == juce::File{})
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else if(!modelInfo.modelFile.existsAsFile())
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Model cannot be found"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
    }
    auto const embeddingModelFile = Rag::getEmbeddingModelFile();
    auto const hasEmbeddingModel = embeddingModelFile.existsAsFile() || std::any_of(processes.cbegin(), processes.cend(), [&](auto const& process)
                                                                                    {
                                                                                        return process.get().getOutputFile() == embeddingModelFile;
                                                                                    });
    auto const rerankerModelFile = Rag::getRerankerModelFile();
    auto const hasRerankerModel = rerankerModelFile.existsAsFile() || std::any_of(processes.cbegin(), processes.cend(), [&](auto const& process)
                                                                                  {
                                                                                      return process.get().getOutputFile() == rerankerModelFile;
                                                                                  });
    mRagDownload.setVisible(!hasEmbeddingModel && !hasRerankerModel);
    resized();
}

void Application::Neuralyzer::SettingsContent::broughtToFront()
{
    postCommandMessage(0);
}

Application::Neuralyzer::SettingsPanel::SettingsPanel(Accessor& accessor)
: mContent(accessor)
{
    setContent(juce::translate("Neuralyzer Settings"), &mContent);
}

Application::Neuralyzer::SettingsPanel::~SettingsPanel()
{
    setContent("", nullptr);
}

void Application::Neuralyzer::SettingsPanel::broughtToFront()
{
    mContent.broughtToFront();
}

ANALYSE_FILE_END
