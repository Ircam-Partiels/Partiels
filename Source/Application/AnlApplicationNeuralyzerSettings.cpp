#include "AnlApplicationNeuralyzerSettings.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>
#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

namespace
{
    static juce::File resolveModelDirectory(juce::File const& root)
    {
#if JUCE_MAC
        return root.getChildFile("Application Support").getChildFile("Ircam").getChildFile("partielsmodels");
#else
        return root.getChildFile("Ircam").getChildFile("partielsmodels");
#endif
    }

    static juce::File getDefaultModelDirectory()
    {
        auto const directory = resolveModelDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
        auto const createResult = directory.createDirectory();
        if(createResult.failed())
        {
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                     .withTitle(juce::translate("Failed to download models"))
                                     .withMessage(juce::translate("The model directory 'DIRPATH' cannot be created: ERROR").replace("DIRPATH", directory.getFullPathName()).replace("ERROR", createResult.getErrorMessage()))
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
            return juce::File();
        }
        return directory;
    }

    static std::vector<juce::File> getInstalledModels()
    {
        std::vector<juce::File> models;
        auto const addFilesFromDirectory = [&](juce::File const& root)
        {
            auto const directory = resolveModelDirectory(root);
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

    static juce::Result downloadModelAndTemplate(nlohmann::json json, juce::File const& output, std::function<bool(int, int)> callback)
    {
        if(callback != nullptr)
        {
            callback(0, 100);
        }

        auto const modelUrl = json.value("model", juce::URL{});
        if(modelUrl.isEmpty())
        {
            return juce::Result::fail(juce::translate("The model configuration does not contain a valid model URL."));
        }

        auto const modelName = json.value("name", juce::String());
        if(modelName.isEmpty())
        {
            return juce::Result::fail(juce::translate("The model configuration does not contain a valid model name."));
        }

        // Download model
        static auto constexpr connectionTimeoutMs = 1000;
        auto const modelTarget = output.getChildFile(modelName + ".gguf");
        std::atomic<bool> shouldContinue{true};
        if(!modelTarget.existsAsFile())
        {
            int statusCode = 0;
            auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                     .withConnectionTimeoutMs(connectionTimeoutMs)
                                     .withStatusCode(&statusCode)
                                     .withProgressCallback([=, sc = &shouldContinue](int avd, int total)
                                                           {
                                                               sc->store(callback != nullptr ? callback(avd, total) : true);
                                                               return sc->load();
                                                           });
            auto const modelResult = Downloader::download(modelUrl, modelTarget, options);
            if(modelResult.failed())
            {
                modelTarget.deleteFile();
                return modelResult;
            }
            if(statusCode < 200 || statusCode >= 300)
            {
                modelTarget.deleteFile();
                return juce::Result::fail(juce::translate("Failed to download model from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", modelUrl.toString(true)).replace("STATUS", juce::String(statusCode)));
            }
            if(!shouldContinue.load())
            {
                modelTarget.deleteFile();
                return juce::Result::fail(juce::translate("Model download cancelled by user."));
            }
        }
        if(callback != nullptr)
        {
            callback(100, 100);
        }

        auto const templateTarget = output.getChildFile(modelName + ".jinja");
        if(templateTarget.existsAsFile())
        {
            return juce::Result::ok();
        }
        // Download template if defined
        auto const templateUrl = json.value("template", juce::URL{});
        if(!templateUrl.isEmpty())
        {
            int statusCode = 0;
            auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                     .withConnectionTimeoutMs(connectionTimeoutMs)
                                     .withStatusCode(&statusCode);
            auto const templateResult = Downloader::download(templateUrl, templateTarget, options);
            if(templateResult.failed())
            {
                templateTarget.deleteFile();
            }
            if(statusCode < 200 || statusCode >= 300)
            {
                templateTarget.deleteFile();
                return juce::Result::fail(juce::translate("Failed to download template from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", templateUrl.toString(true)).replace("STATUS", juce::String(statusCode)));
            }
            if(!shouldContinue.load())
            {
                templateTarget.deleteFile();
                return juce::Result::fail(juce::translate("Model download cancelled by user."));
            }
            return templateResult;
        }

        auto const tokenizerUrl = json.value("tokenizer", juce::URL{});
        if(tokenizerUrl.isEmpty())
        {
            return juce::Result::ok();
        }

        // Download tokenizer and extract template
        int statusCode = 0;
        auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(connectionTimeoutMs)
                                 .withStatusCode(&statusCode);
        auto stream = tokenizerUrl.createInputStream(options);
        if(stream == nullptr)
        {
            templateTarget.deleteFile();
            return juce::Result::fail(juce::translate("Failed to create input stream from URL \"URLPATH\".").replace("URLPATH", tokenizerUrl.toString(true)));
        }
        if(statusCode < 200 || statusCode >= 300)
        {
            templateTarget.deleteFile();
            return juce::Result::fail(juce::translate("Failed to download tokenizer from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", tokenizerUrl.toString(true)).replace("STATUS", juce::String(statusCode)));
        }
        if(!shouldContinue.load())
        {
            templateTarget.deleteFile();
            return juce::Result::fail(juce::translate("Model download cancelled by user."));
        }
        auto const content = stream->readEntireStreamAsString();
        if(content.isEmpty())
        {
            templateTarget.deleteFile();
            return juce::Result::fail(juce::translate("Failed to read content from URL \"URLPATH\".").replace("URLPATH", tokenizerUrl.toString(true)));
        }
        try
        {
            auto const jsonTokenize = nlohmann::json::parse(content.toStdString());
            if(!jsonTokenize.contains("chat_template") || !jsonTokenize.at("chat_template").is_string())
            {
                templateTarget.deleteFile();
                return juce::Result::fail(juce::translate("The tokenizer configuration from URL \"URLPATH\" does not contain a chat template.").replace("URLPATH", tokenizerUrl.toString(true)));
            }
            if(!templateTarget.replaceWithText(juce::String(jsonTokenize.at("chat_template").get<std::string>())))
            {
                templateTarget.deleteFile();
                return juce::Result::fail(juce::translate("Failed to write template file \"FILEPATH\".").replace("FILEPATH", templateTarget.getFullPathName()));
            }
        }
        catch(std::exception const& e)
        {
            templateTarget.deleteFile();
            return juce::Result::fail(juce::translate("Failed to write template file \"FILEPATH\": ERROR.").replace("FILEPATH", templateTarget.getFullPathName()).replace("ERROR", e.what()));
        }
        return juce::Result::ok();
    }

    static nlohmann::json getDefaultModelsInfo()
    {
        try
        {
            auto const jsonContent = nlohmann::json::parse(AnlNeuralyzerData::default_models_json);
            if(jsonContent.contains("models") && jsonContent.at("models").is_array())
            {
                return jsonContent.at("models");
            }
        }
        catch(...)
        {
        }
        return {};
    }
} // namespace

Application::Neuralyzer::SettingsContent::DownloadProcess::DownloadProcess(SettingsContent& owner, nlohmann::json description)
: mOwner(owner)
, mCancelButton(juce::ImageCache::getFromMemory(AnlIconsData::cancel_png, AnlIconsData::cancel_pngSize))
{
    auto const output = getDefaultModelDirectory();
    auto const modelName = description.value("name", juce::String{});
    mModelFile = output.getChildFile(modelName + ".gguf");
    mAsyncProcess = std::async([this, description, output]()
                               {
                                   juce::Thread::setCurrentThreadName("Application::Neuralyzer::Download");
                                   auto result = downloadModelAndTemplate(description, output, [this](int avd, int total)
                                                                          {
                                                                              mProcessProgress.store(total > 0 ? static_cast<float>(avd) / static_cast<float>(total) : 0.0f);
                                                                              return mShouldContinue.load();
                                                                          });
                                   triggerAsyncUpdate();
                                   return result;
                               });
    if(mAsyncProcess.valid())
    {
        startTimer(20);
        addAndMakeVisible(mSeparator);
        addAndMakeVisible(mNameLabel);
        mNameLabel.setText(modelName, juce::dontSendNotification);
        addAndMakeVisible(mProgressBar);
        mProgressBar.setStyle(std::make_optional(juce::ProgressBar::Style::linear));
        mProgressBar.setPercentageDisplay(true);
        addAndMakeVisible(mCancelButton);
        mCancelButton.onClick = [this]()
        {
            mShouldContinue.store(false);
            mCancelButton.setEnabled(false);
        };
    }
    else
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                 .withTitle(juce::translate("Model Download Failed"))
                                 .withMessage(juce::translate("The download of the default model NAME has failed to start. Please try downloading it manually from the model list.").replace("NAME", mModelFile.getFileNameWithoutExtension()))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
    mOwner.postCommandMessage(gDownloadProcessMessageId);
    setSize(400, 37);
}

Application::Neuralyzer::SettingsContent::DownloadProcess::~DownloadProcess()
{
    if(mAsyncProcess.valid())
    {
        mShouldContinue.store(false);
        mAsyncProcess.wait();
        mAsyncProcess.get();
        cancelPendingUpdate();
    }
}

bool Application::Neuralyzer::SettingsContent::DownloadProcess::isRunning() const
{
    return mAsyncProcess.valid();
}

juce::File Application::Neuralyzer::SettingsContent::DownloadProcess::getModelFile() const
{
    return mModelFile;
}

void Application::Neuralyzer::SettingsContent::DownloadProcess::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mSeparator.setBounds(bounds.removeFromTop(1));
    auto nameBounds = bounds.removeFromTop(24);
    mCancelButton.setBounds(nameBounds.removeFromRight(24).reduced(4));
    mNameLabel.setBounds(nameBounds);
    mProgressBar.setBounds(bounds.removeFromTop(12).reduced(4, 0));
    setSize(bounds.getWidth(), bounds.getY());
}

void Application::Neuralyzer::SettingsContent::DownloadProcess::handleAsyncUpdate()
{
    if(!mAsyncProcess.valid())
    {
        return;
    }
    mAsyncProcess.wait();
    auto const result = mAsyncProcess.get();
    if(result.failed())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                 .withTitle(juce::translate("Failed to download the model NAME").replace("NAME", mModelFile.getFileNameWithoutExtension()))
                                 .withMessage(result.getErrorMessage())
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
    else
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Models Downloaded"))
                                 .withMessage(juce::translate("The model NAME have been successfully downloaded. You can now select it in the model list. Would you like to open the Neuralyzer settings panel to select it now?").replace("NAME", mModelFile.getFileNameWithoutExtension()))
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
    stopTimer();
    mOwner.postCommandMessage(gDownloadProcessMessageId);
}

void Application::Neuralyzer::SettingsContent::DownloadProcess::timerCallback()
{
    mEffectiveProgress = static_cast<double>(mProcessProgress.load());
}

Application::Neuralyzer::SettingsContent::SettingsContent(Accessor& accessor)
: mAccessor(accessor)
, mModel(juce::translate("Model"), juce::translate("The model used by the Neuralyzer"), "", {}, nullptr)
, mContextSize(juce::translate("Context Size"), juce::translate("The context size"), "", {}, nullptr)
, mBatchSize(juce::translate("Batch Size"), juce::translate("The batch size"), "", {}, nullptr)
, mModelsDirectory(juce::translate("Models Directory"), juce::translate("Reveal the directory where models are stored"), []()
                   {
                       resolveModelDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)).revealToUser();
                   })
{
    addAndMakeVisible(mModel);
    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
    mModel.entry.setTextWhenNoChoicesAvailable(juce::translate("No model installed"));

    static auto constexpr minSize = 256;
    static auto constexpr maxSize = 65536;

    addAndMakeVisible(mContextSize);
    mContextSize.entry.clear(juce::NotificationType::dontSendNotification);
    mContextSize.entry.setEditableText(true);
    for(auto size = minSize; size <= maxSize; size *= 2)
    {
        mContextSize.entry.addItem(juce::String(size), size);
    }
    mContextSize.entry.getProperties().set("isNumber", true);
    Misc::NumberField::Label::storeProperties(mContextSize.entry.getProperties(), {static_cast<float>(minSize), static_cast<float>(maxSize)}, 1.0f, 0, "");
    mContextSize.entry.onChange = [&, this]()
    {
        auto const size = static_cast<uint32_t>(std::clamp(mContextSize.entry.getText().getIntValue(), minSize, maxSize));
        mAccessor.setAttr<AttrType::contextSize>(size, NotificationType::synchronous);
    };

    addAndMakeVisible(mBatchSize);
    mBatchSize.entry.clear(juce::NotificationType::dontSendNotification);
    mBatchSize.entry.setEditableText(true);
    for(auto size = minSize; size <= maxSize; size *= 2)
    {
        mBatchSize.entry.addItem(juce::String(size), size);
    }
    mBatchSize.entry.getProperties().set("isNumber", true);
    Misc::NumberField::Label::storeProperties(mBatchSize.entry.getProperties(), {static_cast<float>(minSize), static_cast<float>(maxSize)}, 1.0f, 0, "");
    mBatchSize.entry.onChange = [&, this]()
    {
        auto const size = static_cast<uint32_t>(std::clamp(mBatchSize.entry.getText().getIntValue(), minSize, maxSize));
        mAccessor.setAttr<AttrType::batchSize>(size, NotificationType::synchronous);
    };

    addAndMakeVisible(mSeparator);
    mSeparator.setSize(1, 1);

    addAndMakeVisible(mModelsDirectory);

    mModel.entry.onShowPopup = [this]()
    {
        juce::PopupMenu menu;
        auto installedModels = getInstalledModels();
        auto const currentModel = mAccessor.getAttr<AttrType::modelFile>();
        // Add default models
        for(auto const& defaultModel : getDefaultModelsInfo())
        {
            if(defaultModel.contains("name") && defaultModel.at("name").is_string())
            {
                auto const modelName = juce::String(defaultModel.at("name").get<std::string>()).toLowerCase();
                auto const modelAdjective = defaultModel.value("adjective", juce::String());
                auto const modelTitle = modelName + (modelAdjective.isEmpty() ? juce::String() : " (" + modelAdjective + ")");
                auto const localModelIt = std::find_if(installedModels.begin(), installedModels.end(), [&](auto const& installedModel)
                                                       {
                                                           return installedModel.getFileNameWithoutExtension().toLowerCase() == modelName;
                                                       });
                if(localModelIt == installedModels.end())
                {
                    static const auto downloadIndicator = juce::CharPointer_UTF8(" \xf0\x9f\x8c\x90"); // 🌐
                    auto const isNotDownloading = std::none_of(mDownloadProcesses.cbegin(), mDownloadProcesses.cend(), [&](auto const& process)
                                                               {
                                                                   return process->getModelFile().getFileNameWithoutExtension().toLowerCase() == modelName;
                                                               });
                    menu.addItem(modelTitle + juce::String(downloadIndicator), isNotDownloading, false, [=, this]()
                                 {
                                     auto process = std::make_unique<DownloadProcess>(*this, defaultModel);
                                     if(process->isRunning())
                                     {
                                         addAndMakeVisible(*process);
                                         mDownloadProcesses.push_back(std::move(process));
                                         resized();
                                     }
                                 });
                }
                else
                {
                    auto const model = *localModelIt;
                    menu.addItem(modelTitle, true, model == currentModel, [=, this]()
                                 {
                                     mAccessor.setAttr<AttrType::modelFile>(model, NotificationType::synchronous);
                                 });
                    installedModels.erase(localModelIt);
                }
            }
        }

        if(menu.getNumItems() > 0 && !installedModels.empty())
        {
            menu.addSeparator();
        }
        for(auto const& model : installedModels)
        {
            menu.addItem(model.getFileNameWithoutExtension().toLowerCase(), true, model == currentModel, [=]()
                         {
                             mAccessor.setAttr<AttrType::modelFile>(model, NotificationType::synchronous);
                         });
        }

        auto& lf = getLookAndFeel();
        menu.setLookAndFeel(&lf);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mModel.entry).withDeletionCheck(*this), [this](int)
                           {
                               mModel.entry.hidePopup();
                           });
    };

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelFile:
            {
                auto const modelFile = acsr.getAttr<AttrType::modelFile>();
                auto const installedModels = getInstalledModels();
                auto const it = std::find(installedModels.cbegin(), installedModels.cend(), modelFile);
                if(it != installedModels.cend() && it->existsAsFile())
                {
                    mModel.entry.setText(it->getFileNameWithoutExtension().toLowerCase(), juce::NotificationType::dontSendNotification);
                }
                else if(!modelFile.existsAsFile())
                {
                    mModel.entry.setTextWhenNothingSelected(juce::translate("Model cannot be found"));
                    mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
                    mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
                }
                break;
            }
            case AttrType::contextSize:
            {
                auto const contextSize = acsr.getAttr<AttrType::contextSize>();
                if(juce::isPowerOfTwo(contextSize) && contextSize >= static_cast<int>(minSize) && contextSize <= maxSize)
                {
                    mContextSize.entry.setSelectedId(static_cast<int>(contextSize), juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mContextSize.entry.setText(juce::String(contextSize), juce::NotificationType::dontSendNotification);
                }
                break;
            }
            case AttrType::batchSize:
            {
                auto const batchSize = acsr.getAttr<AttrType::batchSize>();
                if(juce::isPowerOfTwo(batchSize) && batchSize >= static_cast<int>(minSize) && batchSize <= maxSize)
                {
                    mBatchSize.entry.setSelectedId(static_cast<int>(batchSize), juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mBatchSize.entry.setText(juce::String(batchSize), juce::NotificationType::dontSendNotification);
                }
                break;
            }
        }
    };

    mTimerClock.callback = [this]()
    {
        mListener.onAttrChanged(mAccessor, AttrType::modelFile);
    };

    setSize(300, 200);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTimerClock.startTimer(500);
}

Application::Neuralyzer::SettingsContent::~SettingsContent()
{
    mTimerClock.stopTimer();
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
    setBounds(mModel);
    setBounds(mContextSize);
    setBounds(mBatchSize);
    setBounds(mSeparator);
    setBounds(mModelsDirectory);
    for(auto const& process : mDownloadProcesses)
    {
        setBounds(*process.get());
    }
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Neuralyzer::SettingsContent::handleCommandMessage(int commandId)
{
    switch(commandId)
    {
        case gDownloadProcessMessageId:
        {
            mDownloadProcesses.erase(std::remove_if(mDownloadProcesses.begin(), mDownloadProcesses.end(), [](auto const& process)
                                                    {
                                                        return !process->isRunning();
                                                    }),
                                     mDownloadProcesses.end());
            resized();
            break;
        }
    }
}

void Application::Neuralyzer::SettingsContent::parentHierarchyChanged()
{
    mTimerClock.callback();
}

void Application::Neuralyzer::SettingsContent::visibilityChanged()
{
    mTimerClock.callback();
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
ANALYSE_FILE_END
