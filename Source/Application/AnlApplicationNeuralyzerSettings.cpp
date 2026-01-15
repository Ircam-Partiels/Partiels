#include "AnlApplicationNeuralyzerSettings.h"
#include "AnlApplicationInstance.h"
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

    static std::vector<juce::File> getInstalledModels()
    {
        std::vector<juce::File> models;
        auto const addFilesFromDirectory = [&](juce::File const& root)
        {
            auto const directory = resolveModelDirectory(root);
            auto const listedModels = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.gguf");
            for(auto const& model : listedModels)
            {
                if(model.withFileExtension(".jinja").existsAsFile())
                {
                    if(std::none_of(models.cbegin(), models.cend(), [&](juce::File const& existingModel)
                                    {
                                        return existingModel.getFullPathName() == model.getFullPathName();
                                    }))
                    {
                        models.push_back(model);
                    }
                }
            }
        };
        addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
        addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::commonApplicationDataDirectory));
        std::sort(models.begin(), models.end());
        return models;
    }

    static juce::Result downloadModelAndTemplate(nlohmann::json json, juce::File const& output, std::function<bool(void)> callback)
    {
        auto const modelUrl = [&]()
        {
            try
            {
                return juce::URL(json.at("model").get<std::string>());
            }
            catch(...)
            {
            }
            return juce::URL();
        }();
        if(modelUrl.isEmpty())
        {
            return juce::Result::fail(juce::translate("The model configuration does not contain a valid model URL."));
        }

        auto const modelName = [&]()
        {
            try
            {
                return juce::String(json.at("name").get<std::string>());
            }
            catch(...)
            {
            }
            return juce::String();
        }();
        if(modelName.isEmpty())
        {
            return juce::Result::fail(juce::translate("The model configuration does not contain a valid model name."));
        }

        auto const templateUrl = [&]()
        {
            try
            {
                return juce::URL(json.at("template").get<std::string>());
            }
            catch(...)
            {
            }
            return juce::URL();
        }();

        auto const tokenizerUrl = [&]()
        {
            try
            {
                return juce::URL(json.at("tokenizer").get<std::string>());
            }
            catch(...)
            {
            }
            return juce::URL();
        }();
        if(templateUrl.isEmpty() && tokenizerUrl.isEmpty())
        {
            return juce::Result::fail(juce::translate("The model configuration does not contain a valid template or tokenizer URL."));
        }

        // Download model
        auto const modelTarget = output.getChildFile(modelName + ".gguf");
        int statusCode = 0;
        std::atomic<bool> shouldContinue{true};
        auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(1000)
                                 .withStatusCode(&statusCode)
                                 .withProgressCallback([=, sc = &shouldContinue](int, int)
                                                       {
                                                           sc->store(callback != nullptr ? callback() : true);
                                                           return sc->load();
                                                       });
        if(!modelTarget.existsAsFile())
        {
            statusCode = 0;
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
                return juce::Result::fail(juce::translate("Model download cancelled by user."));
            }
        }

        auto const templateTarget = output.getChildFile(modelName + ".jinja");
        if(templateTarget.existsAsFile())
        {
            return juce::Result::ok();
        }
        // Download template if defined
        if(!templateUrl.isEmpty())
        {
            statusCode = 0;
            auto const templateResult = Downloader::download(templateUrl, templateTarget, options);
            if(templateResult.failed())
            {
                templateTarget.deleteFile();
            }
            if(statusCode < 200 || statusCode >= 300)
            {
                modelTarget.deleteFile();
                return juce::Result::fail(juce::translate("Failed to download template from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", templateUrl.toString(true)).replace("STATUS", juce::String(statusCode)));
            }
            if(!shouldContinue.load())
            {
                return juce::Result::fail(juce::translate("Model download cancelled by user."));
            }
            return templateResult;
        }

        // Download tokenizer and extract template
        statusCode = 0;
        auto stream = tokenizerUrl.createInputStream(options);
        if(stream == nullptr)
        {
            return juce::Result::fail(juce::translate("Failed to create input stream from URL \"URLPATH\".").replace("URLPATH", tokenizerUrl.toString(true)));
        }
        if(statusCode < 200 || statusCode >= 300)
        {
            modelTarget.deleteFile();
            return juce::Result::fail(juce::translate("Failed to download tokenizer from URL \"URLPATH\": HTTP STATUS.").replace("URLPATH", tokenizerUrl.toString(true)).replace("STATUS", juce::String(statusCode)));
        }
        if(!shouldContinue.load())
        {
            return juce::Result::fail(juce::translate("Model download cancelled by user."));
        }
        if(callback != nullptr && !callback())
        {
            return juce::Result::ok();
        }
        auto const content = stream->readEntireStreamAsString();
        if(content.isEmpty())
        {
            return juce::Result::fail(juce::translate("Failed to read content from URL \"URLPATH\".").replace("URLPATH", tokenizerUrl.toString(true)));
        }
        if(callback != nullptr && !callback())
        {
            return juce::Result::ok();
        }
        try
        {
            auto const jsonTokenize = nlohmann::json::parse(content.toStdString());
            if(!jsonTokenize.contains("chat_template") || !jsonTokenize.at("chat_template").is_string())
            {
                return juce::Result::fail(juce::translate("The tokenizer configuration from URL \"URLPATH\" does not contain a chat template.").replace("URLPATH", tokenizerUrl.toString(true)));
            }
            if(!templateTarget.replaceWithText(juce::String(jsonTokenize.at("chat_template").get<std::string>())))
            {
                return juce::Result::fail(juce::translate("Failed to write template file \"FILEPATH\".").replace("FILEPATH", templateTarget.getFullPathName()));
            }
            return juce::Result::ok();
        }
        catch(std::exception const& e)
        {
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

    static std::optional<uint32_t> getDefaultModelContextSize(juce::String const& modelName)
    {
        for(auto const& defaultModel : getDefaultModelsInfo())
        {
            if(defaultModel.contains("name") && defaultModel.at("name").is_string())
            {
                auto const defaultModelName = juce::String(defaultModel.at("name").get<std::string>());
                if(defaultModelName.equalsIgnoreCase(modelName))
                {
                    auto const value = defaultModel.value("n_ctx", 0u);
                    return value > 0u ? std::make_optional(value) : std::nullopt;
                }
            }
        }
        return {};
    }

    static std::optional<uint32_t> getDefaultModelBatchSize(juce::String const& modelName)
    {
        for(auto const& defaultModel : getDefaultModelsInfo())
        {
            if(defaultModel.contains("name") && defaultModel.at("name").is_string())
            {
                auto const defaultModelName = juce::String(defaultModel.at("name").get<std::string>());
                if(defaultModelName.equalsIgnoreCase(modelName))
                {
                    auto const value = defaultModel.value("n_batch", 0u);
                    return value > 0u ? std::make_optional(value) : std::nullopt;
                }
            }
        }
        return {};
    }

} // namespace

Application::Neuralyzer::SettingsContent::SettingsContent(Accessor& accessor)
: mAccessor(accessor)
, mModel(juce::translate("Model"), juce::translate("The model used by the Neuralyzer"), "", {}, nullptr)
, mContextSize(juce::translate("Context Size"), juce::translate("The context size"), "", {}, nullptr)
, mBatchSize(juce::translate("Batch Size"), juce::translate("The batch size"), "", {}, nullptr)
{
    addAndMakeVisible(mModel);
    mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
    mModel.entry.setTextWhenNoChoicesAvailable(juce::translate("No model installed"));
    addAndMakeVisible(mContextSize);
    addAndMakeVisible(mBatchSize);

    static auto constexpr minSize = 256;
    static auto constexpr maxSize = 65536;
    mContextSize.entry.getProperties().set("isNumber", true);
    Misc::NumberField::Label::storeProperties(mContextSize.entry.getProperties(), {static_cast<float>(minSize), static_cast<float>(maxSize)}, 1.0f, 0, "");
    mContextSize.entry.onChange = [&, this]()
    {
        auto const size = static_cast<uint32_t>(std::clamp(mContextSize.entry.getText().getIntValue(), minSize, maxSize));
        auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
        modelInfo.contextSize = size;
        mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
    };

    mBatchSize.entry.getProperties().set("isNumber", true);
    Misc::NumberField::Label::storeProperties(mBatchSize.entry.getProperties(), {static_cast<float>(minSize), static_cast<float>(maxSize)}, 1.0f, 0, "");
    mBatchSize.entry.onChange = [&, this]()
    {
        auto const size = static_cast<uint32_t>(std::clamp(mBatchSize.entry.getText().getIntValue(), minSize, maxSize));
        auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
        modelInfo.batchSize = size;
        mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
    };

    mModel.entry.onShowPopup = [this]()
    {
        auto const selectModel = [this](juce::File const& file)
        {
            auto modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
            modelInfo.model = file;
            modelInfo.tplt = file.withFileExtension(".jinja");
            auto const defaultContextSize = getDefaultModelContextSize(file.getFileNameWithoutExtension());
            modelInfo.contextSize = defaultContextSize.value_or(0u);
            auto const defaultBatchSize = getDefaultModelBatchSize(file.getFileNameWithoutExtension());
            modelInfo.batchSize = defaultBatchSize.value_or(0u);
            mAccessor.setAttr<AttrType::modelInfo>(modelInfo, NotificationType::synchronous);
        };

        auto const downloadModel = [this](nlohmann::json model)
        {
            auto const output = resolveModelDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
            auto const createResult = output.createDirectory();
            if(createResult.failed())
            {
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                         .withTitle(juce::translate("Failed to download models"))
                                         .withMessage(juce::translate("The model directory 'DIRPATH' cannot be created: ERROR").replace("DIRPATH", output.getFullPathName()).replace("ERROR", createResult.getErrorMessage()))
                                         .withButton(juce::translate("Ok"));
                juce::AlertWindow::showAsync(options, nullptr);
                return;
            }
            auto const modelName = [&]()
            {
                try
                {
                    return juce::String(model.at("name").get<std::string>());
                }
                catch(...)
                {
                }
                return juce::String();
            }();
            mFutureModel = output.getChildFile(modelName + ".gguf");
            mDownloadProcess = std::async([this, model, output]()
                                          {
                                              juce::Thread::setCurrentThreadName("Application::Neuralyzer::Download");
                                              auto const callback = [this]()
                                              {
                                                  return !mShouldQuitDownload.load();
                                              };
                                              auto const result = downloadModelAndTemplate(model, output, callback);
                                              triggerAsyncUpdate();
                                              return result;
                                          });
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                     .withTitle(juce::translate("Model Download Started"))
                                     .withMessage(juce::translate("The download of the default model has started in the background. You will be notified when it is completed."))
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
        };

        juce::PopupMenu menu;
        auto installedModels = getInstalledModels();

        auto const currentModel = mAccessor.getAttr<AttrType::modelInfo>().model;
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
                    menu.addItem(modelTitle + juce::String(downloadIndicator), !mDownloadProcess.valid(), false, [=]()
                                 {
                                     downloadModel(defaultModel);
                                 });
                }
                else
                {
                    auto const model = *localModelIt;
                    menu.addItem(modelTitle, true, model == currentModel, [=]()
                                 {
                                     selectModel(model);
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
                             selectModel(model);
                         });
        }

        auto& lf = getLookAndFeel();
        menu.setLookAndFeel(&lf);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mModel.entry).withDeletionCheck(*this), [this](int)
                           {
                               mModel.entry.hidePopup();
                           });
    };

    auto const propertiesUpdated = [this](bool modelOnly)
    {
        // Update the selected model in the list
        auto const modelInfo = mAccessor.getAttr<AttrType::modelInfo>();
        auto const installedModels = getInstalledModels();
        auto const it = std::find(installedModels.cbegin(), installedModels.cend(), modelInfo.model);
        if(it != installedModels.cend() && it->existsAsFile())
        {
            mModel.entry.setText(it->getFileNameWithoutExtension().toLowerCase(), juce::NotificationType::dontSendNotification);
        }
        else if(!modelInfo.model.existsAsFile())
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Model cannot be found"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mModel.entry.setTextWhenNothingSelected(juce::translate("Select a model"));
            mModel.entry.setSelectedId(0, juce::NotificationType::dontSendNotification);
        }
        if(!modelOnly)
        {
            // Update the context size
            mContextSize.entry.clear(juce::NotificationType::dontSendNotification);
            mContextSize.entry.setEditableText(true);
            auto const defaultContextSize = getDefaultModelContextSize(modelInfo.model.getFileNameWithoutExtension());
            if(defaultContextSize.has_value())
            {
                mContextSize.entry.addItem(juce::String(defaultContextSize.value()) + " (Default)", static_cast<int>(defaultContextSize.value()));
                mContextSize.entry.addSeparator();
            }
            for(auto size = minSize; size <= maxSize; size *= 2)
            {
                if(size != static_cast<int>(defaultContextSize.value_or(0u)))
                {
                    mContextSize.entry.addItem(juce::String(size), size);
                }
            }
            auto const contextSize = modelInfo.contextSize;
            if(defaultContextSize.has_value() && contextSize == defaultContextSize.value())
            {
                mContextSize.entry.setSelectedId(static_cast<int>(contextSize), juce::NotificationType::dontSendNotification);
            }
            else if(juce::isPowerOfTwo(contextSize) && contextSize >= static_cast<int>(minSize) && contextSize <= maxSize)
            {
                mContextSize.entry.setSelectedId(static_cast<int>(contextSize), juce::NotificationType::dontSendNotification);
            }
            else
            {
                mContextSize.entry.setText(juce::String(contextSize), juce::NotificationType::dontSendNotification);
            }
        }
        if(!modelOnly)
        {
            // Update the batch size
            mBatchSize.entry.clear(juce::NotificationType::dontSendNotification);
            mBatchSize.entry.setEditableText(true);
            auto const defaultBatchSize = getDefaultModelBatchSize(modelInfo.model.getFileNameWithoutExtension());
            if(defaultBatchSize.has_value())
            {
                mBatchSize.entry.addItem(juce::String(defaultBatchSize.value()) + " (Default)", static_cast<int>(defaultBatchSize.value()));
                mBatchSize.entry.addSeparator();
            }
            for(auto size = minSize; size <= maxSize; size *= 2)
            {
                if(size != static_cast<int>(defaultBatchSize.value_or(0u)))
                {
                    mBatchSize.entry.addItem(juce::String(size), size);
                }
            }
            auto const batchSize = modelInfo.batchSize;
            if(defaultBatchSize.has_value() && batchSize == defaultBatchSize.value())
            {
                mBatchSize.entry.setSelectedId(static_cast<int>(batchSize), juce::NotificationType::dontSendNotification);
            }
            else if(juce::isPowerOfTwo(batchSize) && batchSize >= static_cast<int>(minSize) && batchSize <= maxSize)
            {
                mBatchSize.entry.setSelectedId(static_cast<int>(batchSize), juce::NotificationType::dontSendNotification);
            }
            else
            {
                mBatchSize.entry.setText(juce::String(batchSize), juce::NotificationType::dontSendNotification);
            }
        }
    };

    mListener.onAttrChanged = [=]([[maybe_unused]] Accessor const& acsr, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::modelInfo:
            {
                propertiesUpdated(false);
                break;
            }
        }
    };

    mTimerClock.callback = [=]()
    {
        propertiesUpdated(true);
    };

    setSize(300, 200);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTimerClock.startTimer(500);
}

Application::Neuralyzer::SettingsContent::~SettingsContent()
{
    if(mDownloadProcess.valid())
    {
        mShouldQuitDownload.store(true);
        mDownloadProcess.wait();
        mDownloadProcess.get();
        cancelPendingUpdate();
    }
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
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Neuralyzer::SettingsContent::parentHierarchyChanged()
{
    mTimerClock.callback();
}

void Application::Neuralyzer::SettingsContent::visibilityChanged()
{
    mTimerClock.callback();
}

void Application::Neuralyzer::SettingsContent::handleAsyncUpdate()
{
    if(!mDownloadProcess.valid())
    {
        return;
    }
    mDownloadProcess.wait();
    auto const result = mDownloadProcess.get();
    if(result.failed())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                 .withTitle(juce::translate("Failed to download the model NAME").replace("NAME", mFutureModel.getFileNameWithoutExtension()))
                                 .withMessage(result.getErrorMessage())
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    }
    else
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Models Downloaded"))
                                 .withMessage(juce::translate("The model NAME have been successfully downloaded. You can now select it in the model list. Would you like to open the Neuralyzer settings panel to select it now?").replace("NAME", mFutureModel.getFileNameWithoutExtension()))
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
    mFutureModel = juce::File{};
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
