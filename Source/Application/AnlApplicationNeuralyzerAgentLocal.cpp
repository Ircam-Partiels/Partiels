#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationInstance.h"
#include <AnlResourceData.h>

// The audio formats decoded by the multimodal projectors of llama.cpp (mtmd)
static bool isMediaAudioFileForInjection(juce::File const& file)
{
    return file.hasFileExtension("wav;mp3;flac;riff");
}

static bool isMediaImageFileForInjection(juce::File const& file)
{
    return file.hasFileExtension("png;jpg;jpeg");
}

static bool isMediaTextFile(juce::File const& file)
{
    return file.hasFileExtension("txt;md;json;xml;csv;lab;cue");
}

ANALYSE_FILE_BEGIN

static common_chat_msg formatToolCall(common_chat_msg const& message, bool appendInstructions)
{
    auto const unquote = [](std::string const& str) -> std::string
    {
        return juce::String(str).replace(R"(\")", R"(")").toStdString();
    };

    static auto constexpr fmtErrorMsg = "\n\nWarning: The tool call had error. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps.";

    auto fmtMsg = message;
    fmtMsg.content.clear();
    fmtMsg.content += "Tool Call Result:\n";
    fmtMsg.content += "- ID: " + message.tool_call_id + "\n";
    fmtMsg.content += "- Name: " + message.tool_name + "\n";
    nlohmann::json parsed;
    try
    {
        parsed = nlohmann::json::parse(message.content);
    }
    catch(std::exception const& e)
    {
        fmtMsg.content += "- Status: ERROR\n";
        fmtMsg.content += "- Message:\nException during parsing: " + std::string(e.what());
        if(appendInstructions)
        {
            fmtMsg.content += fmtErrorMsg;
        }
        return fmtMsg;
    }
    catch(...)
    {
        fmtMsg.content += "- Status: ERROR\n";
        fmtMsg.content += "- Message:\nException during parsing: Unknown";
        if(appendInstructions)
        {
            fmtMsg.content += fmtErrorMsg;
        }
        return fmtMsg;
    }

    if(!parsed.is_object())
    {
        fmtMsg.content += "- Status: ERROR\n";
        fmtMsg.content += "- Message:\nInvalid response format: " + parsed.dump();
        if(appendInstructions)
        {
            fmtMsg.content += fmtErrorMsg;
        }
        return fmtMsg;
    }

    if(parsed.contains("error"))
    {
        fmtMsg.content += "- Status: ERROR\n";
        if(parsed.at("error").is_object())
        {
            auto const& error = parsed.at("error");
            if(error.contains("code") && error.at("code").is_number())
            {
                fmtMsg.content += "- Code: " + std::to_string(error.at("code").get<int>()) + "\n";
            }
            if(error.contains("message") && error.at("message").is_string())
            {
                fmtMsg.content += "- Message:\n" + unquote(error.at("message").get<std::string>()) + "\n";
            }
        }
        else
        {
            fmtMsg.content += "- Message:\n" + unquote(parsed.at("error").dump()) + "\n";
        }
        if(appendInstructions)
        {
            fmtMsg.content += fmtErrorMsg;
        }
        return fmtMsg;
    }

    // The content may be missing, empty or without text
    auto const text = [&]() -> std::string
    {
        auto const content = parsed.find("content");
        if(content == parsed.end() || !content->is_array() || content->empty() || !content->front().is_object())
        {
            return "(empty)";
        }
        auto const textIt = content->front().find("text");
        return textIt != content->front().end() && textIt->is_string() ? unquote(textIt->get<std::string>()) : "(empty)";
    }();
    if(parsed.value("isError", false))
    {
        fmtMsg.content += "- Status: FAILED\n";
        fmtMsg.content += "- Message:\n" + text + "\n";
        if(appendInstructions)
        {
            static auto constexpr failErrorMsg = "\n\nWarning: The tool call had error. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. If necessary, ensure your response is accurate based on the current state of the document.";
            fmtMsg.content += failErrorMsg;
        }
    }
    else
    {
        fmtMsg.content += "- Status: SUCCESSFUL\n";
        fmtMsg.content += "- Result:\n" + text + "\n";
        if(appendInstructions)
        {
            static auto constexpr successMsg = "\n\nBased on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document.";
            fmtMsg.content += successMsg;
        }
    }
    return fmtMsg;
}

juce::File Application::Neuralyzer::AgentLocal::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Models");
}

std::set<juce::File> Application::Neuralyzer::AgentLocal::getAvailableModels()
{
    std::set<juce::File> models;
    auto const addModelsFromDirectory = [&](juce::File const& root)
    {
        auto const listedModels = root.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.gguf");
        for(auto const& model : listedModels)
        {
            auto const fileName = model.getFileName();
            if(!fileName.containsIgnoreCase("mmproj") && !fileName.containsIgnoreCase("mtp"))
            {
                models.insert(model);
            }
        }
    };

    // Partiels data directories
    auto const addModelsFromPartielsDirectory = [&](juce::File const& root)
    {
        auto const modelsDirectory = resolveDirectory(root).getChildFile("Models");
        for(auto const& repository : modelsDirectory.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false))
        {
            addModelsFromDirectory(repository);
        }
    };

    addModelsFromPartielsDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
    addModelsFromPartielsDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::commonApplicationDataDirectory));

    // HuggingFace data directories
    auto const addModelsFromHFDirectory = [&](juce::File const& root)
    {
        for(auto const& repository : root.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false, "models--*"))
        {
            addModelsFromDirectory(repository.getChildFile("snapshots"));
        }
    };
    auto const home = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    addModelsFromHFDirectory(home.getChildFile(".cache/huggingface/hub"));
    addModelsFromHFDirectory(home.getChildFile("huggingface/hub"));
    auto const hubCache = juce::SystemStats::getEnvironmentVariable("HF_HUB_CACHE", {});
    if(hubCache.isNotEmpty())
    {
        addModelsFromHFDirectory(juce::File(hubCache));
    }
    auto const hfHome = juce::SystemStats::getEnvironmentVariable("HF_HOME", {});
    if(hfHome.isNotEmpty())
    {
        addModelsFromHFDirectory(juce::File(hfHome).getChildFile("hub"));
    }
    return models;
}

juce::File Application::Neuralyzer::AgentLocal::getProjectorForModel(juce::File const& modelFile)
{
    auto const directory = modelFile.getParentDirectory();
    auto const candidates = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.gguf");
    std::set<juce::File> projectors;
    for(auto const& candidate : candidates)
    {
        if(candidate.getFileName().containsIgnoreCase("mmproj"))
        {
            projectors.insert(candidate);
        }
    }
    return projectors.empty() ? juce::File{} : *projectors.begin();
}

juce::File Application::Neuralyzer::AgentLocal::getMtpForModel(juce::File const& modelFile)
{
    auto const directory = modelFile.getParentDirectory();
    auto const candidates = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.gguf");
    std::set<juce::File> mtps;
    for(auto const& candidate : candidates)
    {
        auto const fileName = candidate.getFileName();
        if(fileName.containsIgnoreCase("mtp") && !fileName.containsIgnoreCase("mmproj"))
        {
            mtps.insert(candidate);
        }
    }
    return mtps.empty() ? juce::File{} : *mtps.begin();
}

std::vector<Application::Neuralyzer::AgentLocal::ModelBundle> Application::Neuralyzer::AgentLocal::getDefaultModelBundles()
{
    std::vector<ModelBundle> bundles;
    {
        // clang-format off
        bundles.push_back({
                                 juce::URL("https://huggingface.co/unsloth/Qwen3.5-9B-GGUF/resolve/main/Qwen3.5-9B-Q4_K_M.gguf")
                               , getDefaultModelDirectory().getChildFile("Qwen3.5-9B-Q4_K_M").getChildFile("Qwen3.5-9B-Q4_K_M.gguf")
                               , juce::URL("https://huggingface.co/unsloth/Qwen3.5-9B-GGUF/resolve/main/mmproj-BF16.gguf")
                               , getDefaultModelDirectory().getChildFile("Qwen3.5-9B-Q4_K_M").getChildFile("mmproj-BF16.gguf")
                               , juce::URL()
                               , juce::File()
                           });
        // clang-format on
    }
    {
        // clang-format off
        bundles.push_back({
                              juce::URL("https://huggingface.co/unsloth/Qwen3.8-27B-GGUF/resolve/main/Qwen3.8-27B-UD-Q4_K_M.gguf")
                            , getDefaultModelDirectory().getChildFile("Qwen3.8-27B-UD-Q4_K_M").getChildFile("Qwen3.8-27B-UD-Q4_K_M.gguf")
                            , juce::URL("https://huggingface.co/unsloth/Qwen3.8-27B-GGUF/resolve/main/mmproj-BF16.gguf")
                            , getDefaultModelDirectory().getChildFile("Qwen3.8-27B-UD-Q4_K_M").getChildFile("mmproj-BF16.gguf")
                            , juce::URL("https://huggingface.co/unsloth/Qwen3.8-27B-GGUF/resolve/main/MTP/mtp-Qwen3.8-27B-Q4_0.gguf")
                            , getDefaultModelDirectory().getChildFile("Qwen3.8-27B-UD-Q4_K_M").getChildFile("mtp-Qwen3.8-27B-Q4_0.gguf")
                        });
        // clang-format on
    }
    return bundles;
}

Application::Neuralyzer::AgentLocal::ModelBundle Application::Neuralyzer::AgentLocal::getDefaultModelBundle()
{
    static auto const defaultBundles = getDefaultModelBundles();
    return defaultBundles.empty() ? ModelBundle{} : defaultBundles.at(0_z);
}

void Application::Neuralyzer::AgentLocal::downloadModelBundle(ModelBundle const& bundle, bool warnIfFailed)
{
    auto const callback = [=](Downloader::Process const& process)
    {
        auto const result = process.getResult();
        auto const file = process.getOutputFile();
        if(result.failed())
        {
            if(warnIfFailed)
            {
                auto const options = juce::MessageBoxOptions()
                                         .withIconType(juce::AlertWindow::AlertIconType::WarningIcon)
                                         .withTitle(juce::translate("Failed to download the model MDLNAME").replace("MDLNAME", file.getFileNameWithoutExtension()))
                                         .withMessage(result.getErrorMessage())
                                         .withButton(juce::translate("Ok"), 0);
                juce::AlertWindow::showAsync(options, nullptr);
            }
            return;
        }

        auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
        // Ensure both the model and the projector are available and that the optional MTP model is not still downloading
        for(auto const& requiredFile : {bundle.modelFile, bundle.projectorFile})
        {
            if(requiredFile != file && (!requiredFile.existsAsFile() || downloader.isDownloading(requiredFile)))
            {
                return;
            }
        }
        if(bundle.mtpFile != juce::File() && bundle.mtpFile != file && downloader.isDownloading(bundle.mtpFile))
        {
            return;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Model Downloaded"))
                                 .withMessage(juce::translate("Model MDLNAME and its projector have been downloaded. Would you like to open the agent settings to select the model now?").replace("MDLNAME", bundle.modelFile.getFileNameWithoutExtension()))
                                 .withButton(juce::translate("Open"), 1)
                                 .withButton(juce::translate("Close"), 0);
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
    };

    auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
    downloader.startIfNecessary(bundle.modelFile, bundle.modelUrl, callback);
    downloader.startIfNecessary(bundle.projectorFile, bundle.projectorUrl, callback);
    if(bundle.mtpFile != juce::File())
    {
        downloader.startIfNecessary(bundle.mtpFile, bundle.mtpUrl, callback);
    }
}

void Application::Neuralyzer::AgentLocal::downloadDefaultModelIfNecessary()
{
    auto const defaultBundle = getDefaultModelBundle();
    if(!defaultBundle.modelFile.existsAsFile() && !defaultBundle.projectorFile.existsAsFile() && !getAvailableModels().empty())
    {
        return;
    }
    downloadModelBundle(defaultBundle, false);
}

Application::Neuralyzer::AgentLocal::AgentLocal(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine)
: mMcpDispatcher(mcpDispatcher)
, mRagEngine(ragEngine)
{
    mMcpMethods.readFilesFn = std::bind(&AgentLocal::readFiles, this, std::placeholders::_1);
    mMcpMethods.searchDocsFn = [this](std::string const& query, size_t maxNumResources)
    {
        return mRagEngine.searchDocs(query, maxNumResources);
    };
    mMcpMethods.loadDocsFn = [this](std::vector<std::string> const& ids)
    {
        return mRagEngine.loadDocs(ids);
    };
    mTools = mMcpDispatcher.getToolList(mMcpMethods);
}

void Application::Neuralyzer::AgentLocal::setNotifyCallback(std::function<void()> callback)
{
    std::unique_lock<std::mutex> lock(mNotifyMutex);
    mNotifyCallback = std::move(callback);
}

void Application::Neuralyzer::AgentLocal::notifyStateChanged()
{
    std::function<void()> callback;
    {
        std::unique_lock<std::mutex> lock(mNotifyMutex);
        callback = mNotifyCallback;
    }
    if(callback)
    {
        callback();
    }
}

Application::Neuralyzer::Mcp::Dispatcher& Application::Neuralyzer::AgentLocal::getMcpDispatcher()
{
    return mMcpDispatcher;
}

std::vector<common_chat_msg> Application::Neuralyzer::AgentLocal::getHistory() const
{
    std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
    return mMessages;
}

juce::Result Application::Neuralyzer::AgentLocal::initializeModel(ModelInfo const& info)
{
    MiscDebug("Application::Neuralyzer::AgentLocal", "Initialize...");
    mEngine.reset();
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
    }
    notifyStateChanged();
    auto const hasModel = info.modelFile.existsAsFile();
    auto const result = mEngine.initialize(info, hasModel ? getProjectorForModel(info.modelFile) : juce::File{}, hasModel ? getMtpForModel(info.modelFile) : juce::File{});
    notifyStateChanged();
    return result;
}

juce::Result Application::Neuralyzer::AgentLocal::resetModel()
{
    mEngine.reset();
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
    }
    notifyStateChanged();
    return juce::Result::ok();
}

common_chat_msg Application::Neuralyzer::AgentLocal::performInference(bool useThinkingMode)
{
    return mEngine.generate(getHistory(), mTools, useThinkingMode, [this]()
                            {
                                notifyStateChanged();
                            },
                            [this](std::string const& response)
                            {
                                std::unique_lock<std::mutex> lock(mTemporaryMutex);
                                mTempResponse = response;
                            });
}

juce::Result Application::Neuralyzer::AgentLocal::sendQuery(juce::String const& prompt, bool useThinkingMode)
{
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }

    if(!mEngine.isInitialized())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Summarize the conversation if there is less than 20% of the context available and if the history (without the
    // system prompt that cannot be summarized) occupies a significant part of the context, otherwise the summary would
    // not free enough space and the user has to reset the history when the context is full
    auto const ctxCapacity = mEngine.getContextCapacity();
    auto const ctxUsed = mEngine.getContextSize();
    if(static_cast<double>(ctxUsed) / static_cast<double>(ctxCapacity) > 0.8)
    {
        auto const systemPromptSize = mEngine.getSystemPromptSize(getHistory(), mTools, useThinkingMode);
        auto const historySize = ctxUsed > systemPromptSize ? ctxUsed - systemPromptSize : 0_z;
        MiscDebug("Application::Neuralyzer::AgentLocal", "Context usage above 80% with a history of " + juce::String(historySize) + " tokens");
        if(historySize > ctxCapacity / 4_z)
        {
            // The summary is optional: if it fails (the history is restored), the query is sent anyway and the user is
            // invited to reset the history if the context is full
            auto const summarizeResult = summarizeSession(useThinkingMode);
            if(summarizeResult.failed())
            {
                if(mShouldQuit.load())
                {
                    return summarizeResult;
                }
                MiscDebug("Application::Neuralyzer::AgentLocal", "The query is sent without summary - " + summarizeResult.getErrorMessage());
            }
        }
    }

    common_chat_msg message;
    message.role = "user";
    message.content = prompt.toStdString();

    // Keep the history to restore it if the query fails: the restore point follows the completed tool calls so the
    // history stays consistent with the modifications of the document (the context memory follows the history with
    // the next inference)
    auto restorePoint = getHistory();
    auto const restoreHistory = [&]()
    {
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages = restorePoint;
        }
        notifyStateChanged();
    };

    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.push_back(std::move(message));
    }
    notifyStateChanged();

    static auto constexpr maxIterations = 25_z; // Prevent infinite loops
    for(auto iteration = 0_z; iteration < maxIterations; ++iteration)
    {
        if(mShouldQuit.load())
        {
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        common_chat_msg response;
        try
        {
            response = performInference(useThinkingMode);
        }
        catch(juce::String const& e)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + e);
            return juce::Result::fail(e);
        }
        catch(std::exception const& e)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + juce::String(e.what()));
            return juce::Result::fail(juce::String::fromUTF8(e.what()));
        }
        catch(...)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - Unknown error");
            return juce::Result::fail(juce::translate("Unknown error"));
        }

        if(mShouldQuit.load())
        {
            restoreHistory();
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        if(response.empty())
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - No model response");
            return juce::Result::fail(juce::translate("No model response"));
        }

        auto const toolCalls = response.tool_calls;
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.push_back(std::move(response));
        }
        notifyStateChanged();

        // Without tool calls, the response is the final answer
        if(toolCalls.empty())
        {
            return juce::Result::ok();
        }

        for(auto const& toolCall : toolCalls)
        {
            auto toolMessage = formatToolCall(mMcpDispatcher.callTool(mMcpMethods, toolCall), false);
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.push_back(std::move(toolMessage));
        }
        restorePoint = getHistory();
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted"));
    }
    return juce::Result::fail(juce::translate("Maximum number of iterations reached"));
}

juce::Result Application::Neuralyzer::AgentLocal::addMedia(juce::File const& file)
{
    if(file == juce::File{} || !file.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The media file does not exist: " + file.getFullPathName());
        return juce::Result::fail(juce::translate("The media file does not exist: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }
    if(!mEngine.isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    if(!mEngine.supportsMedia())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The current model does not expose a multimodal context.");
        return juce::Result::fail(juce::translate("The current model does not support media input."));
    }
    // The file is checked now for an immediate feedback, but the media is evaluated with the next inference
    auto const result = mEngine.checkMedia(file);
    if(result.failed())
    {
        return result;
    }
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.push_back(EngineLocal::createMediaMessage(file));
    }
    notifyStateChanged();
    return juce::Result::ok();
}

nlohmann::json Application::Neuralyzer::AgentLocal::readFiles(std::vector<std::string> const& filePaths)
{
    nlohmann::json response;
    response["isError"] = false;
    response["content"] = nlohmann::json::array();

    std::string message = "File loading results:\n";
    for(auto const& filePath : filePaths)
    {
        message += "\n--------\n";
        message += "Path: '" + filePath + "'\n";
        message += "Result: ";
        // The path is checked before creating the file, which requires an absolute path
        if(!juce::File::isAbsolutePath(juce::String::fromUTF8(filePath.c_str())))
        {
            message += "The path is not absolute.\n";
            response["isError"] = true;
            continue;
        }
        juce::File const file(filePath);
        if(!file.existsAsFile())
        {
            message += "The file does not exist.";
            response["isError"] = true;
        }
        else if(!file.hasReadAccess())
        {
            message += "The file does not have read access.";
            response["isError"] = true;
        }
        else if(isMediaAudioFileForInjection(file))
        {
            auto const result = addMedia(file);
            if(result.failed())
            {
                message += "The audio file cannot be injected into the context: " + result.getErrorMessage().toStdString();
                response["isError"] = true;
            }
            else
            {
                message += "The audio file has been injected into the context.";
            }
        }
        else if(isMediaImageFileForInjection(file))
        {
            auto const result = addMedia(file);
            if(result.failed())
            {
                message += "The image file cannot be injected into the context: " + result.getErrorMessage().toStdString();
                response["isError"] = true;
            }
            else
            {
                message += "The image file has been injected into the context.";
            }
        }
        else if(isMediaTextFile(file))
        {
            static auto constexpr maxChars = 32000;
            auto const text = file.loadFileAsString();
            auto const truncated = text.length() > maxChars;
            if(truncated)
            {
                message += "The truncated text file content (up to 32000 chars) is:\n";
                message += text.substring(0, maxChars).toStdString() + "\n...";
            }
            else
            {
                message += "The text file content is:\n";
                message += text.toStdString();
            }
        }
        else
        {
            message += "The file '" + filePath + "' is not supported.";
            response["isError"] = true;
        }
        message += "\n";
    }

    nlohmann::json content;
    content["type"] = "text";
    content["text"] = message;
    response["content"].push_back(content);
    return response;
}

juce::Result Application::Neuralyzer::AgentLocal::startSession()
{
    if(!mEngine.isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    mEngine.clearContext();

    common_chat_msg message;
    message.role = "system";
    message.content = mMcpDispatcher.getInstructions();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages = {std::move(message)};
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    notifyStateChanged();
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::loadSession(juce::File const& sessionFile)
{
    if(!mEngine.isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file provided."));
    }
    if(sessionFile.getSize() <= 0)
    {
        return juce::Result::fail(juce::translate("The session file is empty."));
    }

    std::vector<common_chat_msg> messages;
    std::string version;
    int contextVersion = 0;
    try
    {
        auto const root = nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format"));
        }

        messages = common_chat_msgs_parse_oaicompat(common_json::parse(root.at("messages").dump()));
        version = root.value("version", std::string{});
        contextVersion = root.value("contextVersion", 0);
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to load session: ") + juce::String(e.what()));
    }
    // Use the latest instructions (the system message is restored if missing)
    if(messages.empty() || messages.front().role != "system")
    {
        common_chat_msg message;
        message.role = "system";
        messages.insert(messages.begin(), std::move(message));
    }
    messages.front().content = mMcpDispatcher.getInstructions();

    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages = std::move(messages);
    }

    mEngine.clearContext();
    // The state of the context memory is only restored if it matches the instructions and the format of the engine
    auto const contextFile = sessionFile.withFileExtension(".ctx");
    if(version == mMcpDispatcher.getUuid() && contextVersion == EngineLocal::getStateVersion() && contextFile.getSize() > 0)
    {
        auto const result = mEngine.loadState(contextFile);
        if(result.failed())
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "Context state not restored - " + result.getErrorMessage());
        }
    }
    notifyStateChanged();
    MiscDebug("Application::Neuralyzer::AgentLocal", "Session loaded " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::saveSession(juce::File const& sessionFile)
{
    if(!mEngine.isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file provided."));
    }
    if(!sessionFile.getParentDirectory().createDirectory())
    {
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", sessionFile.getParentDirectory().getFullPathName()));
    }

    auto const messages = getHistory();
    try
    {
        nlohmann::json root;
        root["version"] = mMcpDispatcher.getUuid();
        root["contextVersion"] = EngineLocal::getStateVersion();
        for(auto const& message : messages)
        {
            root["messages"].push_back(nlohmann::json::parse(message.to_json_oaicompat().dump()));
        }

        if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
        {
            return juce::Result::fail(juce::translate("Failed to write session file"));
        }
        MiscDebug("Application::Neuralyzer::AgentLocal", "Session saved with " + juce::String(messages.size()) + " messages");
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to save session: ") + juce::String(e.what()));
    }
    return mEngine.saveState(sessionFile.withFileExtension(".ctx"));
}

juce::Result Application::Neuralyzer::AgentLocal::summarizeSession(bool useThinkingMode)
{
    if(!mEngine.isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto const ctxCapacity = mEngine.getContextCapacity();
    auto const ctxUsed = mEngine.getContextSize();
    auto const ctxAvailable = ctxCapacity > ctxUsed ? ctxCapacity - ctxUsed : 0_z;
    auto const summaryPrompt = juce::String("Summarize our conversation in a concise way that preserves important details and context. This summary will replace the earlier messages in the context to free up space, so it should capture key points, decisions made, and any relevant information. The summary should be brief (MAXNUMTOKENS tokens max) but informative, allowing us to continue our discussion without losing important context.").replace("MAXNUMTOKENS", juce::String(ctxAvailable / 2_z));

    common_chat_msg message;
    message.role = "user";
    message.content = summaryPrompt.toStdString();

    // The previous history is restored if the summary fails (the context memory follows the history with the next
    // inference)
    auto const previousMessages = getHistory();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.push_back(message);
    }
    auto const restoreHistory = [&](juce::String const& error)
    {
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages = previousMessages;
        }
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", "Summary failed - " + error);
        return juce::Result::fail(error);
    };

    common_chat_msg response;
    try
    {
        response = performInference(useThinkingMode);
    }
    catch(juce::String const& e)
    {
        return restoreHistory(e);
    }
    catch(std::exception const& e)
    {
        return restoreHistory(juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        return restoreHistory(juce::translate("Unknown error"));
    }

    if(mShouldQuit.load())
    {
        return restoreHistory(juce::translate("Operation aborted"));
    }

    // Only the text of the summary is kept (the tool calls cannot be answered)
    if(response.content.empty())
    {
        return restoreHistory(juce::translate("No model response"));
    }
    common_chat_msg summary;
    summary.role = "assistant";
    summary.content = response.content;

    // The history is replaced by the system message, the summary request and the summary (a template may require a
    // user message before an assistant message)
    MiscWeakAssert(!previousMessages.empty() && previousMessages.front().role == "system");
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages = {previousMessages.front(), std::move(message), std::move(summary)};
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    notifyStateChanged();
    return juce::Result::ok();
}

float Application::Neuralyzer::AgentLocal::getContextCapacityUsage() const
{
    return mEngine.getContextUsage();
}

juce::String Application::Neuralyzer::AgentLocal::getTemporaryResponse() const
{
    std::unique_lock<std::mutex> lock(mTemporaryMutex);
    return juce::String(mTempResponse);
}

void Application::Neuralyzer::AgentLocal::setShouldQuit(bool state)
{
    mShouldQuit.store(state);
}

bool Application::Neuralyzer::AgentLocal::shouldQuit() const
{
    return mShouldQuit.load();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::AgentLocal::getModelInfo() const
{
    return mEngine.getModelInfo();
}

class ToolCallFormatTest
: public juce::UnitTest
{
public:
    ToolCallFormatTest()
    : juce::UnitTest("Neuralyzer", "Application")
    {
    }

    ~ToolCallFormatTest() override = default;

    void runTest() override
    {
        beginTest("Tool Call Format");
        {
            auto const formatContent = [](std::string const& content)
            {
                common_chat_msg message;
                message.role = "tool";
                message.tool_name = "my_method";
                message.content = content;
                return formatToolCall(message, false).content;
            };
            expect(formatContent(R"({"content":[{"type":"text","text":"result"}]})").find("Status: SUCCESSFUL\n- Result:\nresult") != std::string::npos);
            expect(formatContent(R"({"content":[{"type":"text","text":"failure"}],"isError":true})").find("Status: FAILED\n- Message:\nfailure") != std::string::npos);
            expect(formatContent(R"({"content":[]})").find("(empty)") != std::string::npos);
            expect(formatContent(R"({"isError":false})").find("(empty)") != std::string::npos);
            expect(formatContent(R"({"content":[{"type":"image"}]})").find("(empty)") != std::string::npos);
            expect(formatContent(R"({"content":["text"]})").find("(empty)") != std::string::npos);
            expect(formatContent(R"({"content":{"text":"text"}})").find("(empty)") != std::string::npos);
            expect(formatContent(R"(["content"])").find("Status: ERROR") != std::string::npos);
            expect(formatContent(R"({"error":{"code":-1,"message":"error"}})").find("Status: ERROR\n- Code: -1\n- Message:\nerror") != std::string::npos);
            expect(formatContent("not json").find("Status: ERROR") != std::string::npos);
        }
    }
};

static ToolCallFormatTest toolCallFormatTest;

// Runs a multi-turn conversation with a local model to check the reuse of the context memory.
// The test is only enabled if the environment variable PARTIELS_NEURALYZER_TEST_MODEL is set
// with the path of a model file (the multimodal projector of the model is used if available).
class AgentLocalTest
: public juce::UnitTest
{
public:
    AgentLocalTest()
    : juce::UnitTest("Neuralyzer Agent Local", "Application")
    {
    }

    ~AgentLocalTest() override = default;

    void runTest() override
    {
        auto const modelPath = juce::SystemStats::getEnvironmentVariable("PARTIELS_NEURALYZER_TEST_MODEL", {});
        if(modelPath.isEmpty())
        {
            return;
        }

        beginTest("Initialization");
        Application::Neuralyzer::Mcp::Dispatcher dispatcher(Application::Instance::get());
        Application::Neuralyzer::Rag::Engine engine;
        Application::Neuralyzer::AgentLocal agent(dispatcher, engine);
        Application::Neuralyzer::ModelInfo info;
        info.modelFile = juce::File(modelPath);
        info.contextSize = 32768;
        auto const initResult = agent.initializeModel(info);
        expect(initResult.wasOk(), initResult.getErrorMessage());
        if(initResult.failed())
        {
            return;
        }
        expect(agent.startSession().wasOk());

        auto const ask = [&](juce::String const& prompt, juce::String const& expected, bool useThinkingMode = true)
        {
            auto const start = juce::Time::getMillisecondCounterHiRes();
            auto const result = agent.sendQuery(prompt + " Answer briefly and do not use tools.", useThinkingMode);
            auto const duration = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
            auto const history = agent.getHistory();
            auto const answer = history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str());
            logMessage(prompt + " -> " + answer + " (" + juce::String(duration, 2) + "s, context usage " + juce::String(agent.getContextCapacityUsage(), 3) + ")");
            expect(result.wasOk(), result.getErrorMessage());
            expect(expected.isEmpty() || answer.containsIgnoreCase(expected), answer);
        };

        beginTest("Multi-turn");
        // The model may refuse to answer about personal information (the instructions restrict the agent to Partiels),
        // so the questions are about the document
        ask("The Partiels document I am working on is named Zorglub and it contains 47 tracks.", {});
        ask("What is the name of the Partiels document I am working on?", "Zorglub");
        ask("How many tracks would the document contain if I added 3 tracks?", "50");

        beginTest("Session");
        auto const sessionFile = juce::File::createTempFile("ptldoc");
        expect(agent.saveSession(sessionFile).wasOk());
        expect(agent.loadSession(sessionFile).wasOk());
        ask("How many tracks does the document contain?", "47");
        sessionFile.deleteFile();
        sessionFile.withFileExtension(".ctx").deleteFile();

        // The media files of the history are reloaded with each query so the file must exist until the end of the test
        juce::TemporaryFile imageFile(".png");
        if(Application::Neuralyzer::AgentLocal::getProjectorForModel(info.modelFile) != juce::File{})
        {
            beginTest("Media");
            expect(imageFile.getFile().replaceWithData(AnlResourceData::Ircamlogo_png, static_cast<size_t>(AnlResourceData::Ircamlogo_pngSize)));
            expect(agent.addMedia(imageFile.getFile()).wasOk());
            ask("Which institute is named in the image I just gave you?", "IRCAM");
            ask("What is the name of the Partiels document I am working on?", "Zorglub");
            ask("What colors were in the image?", {});
        }

        // The template renders the tool call from the parsed message, so the next prompt can diverge from the generated
        // tokens in the context memory: the context memory is rolled back (with the checkpoint for the hybrid models).
        // The tool reads a file because the tools of the document cannot be used without the application window.
        beginTest("Tool Call");
        {
            juce::TemporaryFile textFile(".txt");
            expect(textFile.getFile().replaceWithText("The secret code of the studio is FANTASIO-1946."));
            auto const path = textFile.getFile().getFullPathName();
            auto const result = agent.sendQuery("Call the tool read_files once to read the file " + path + ", then tell me the secret code written in the file.", true);
            auto const history = agent.getHistory();
            logMessage("Tool call -> " + (history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str())) + " (context usage " + juce::String(agent.getContextCapacityUsage(), 3) + ")");
            expect(result.wasOk(), result.getErrorMessage());
            auto const hasToolCall = std::any_of(history.cbegin(), history.cend(), [](auto const& message)
                                                 {
                                                     return std::any_of(message.tool_calls.cbegin(), message.tool_calls.cend(), [](auto const& toolCall)
                                                                        {
                                                                            return toolCall.name == "read_files";
                                                                        });
                                                 });
            auto const hasToolResult = std::any_of(history.cbegin(), history.cend(), [](auto const& message)
                                                   {
                                                       return message.role == "tool" && message.tool_name == "read_files";
                                                   });
            expect(hasToolCall && hasToolResult);
            expect(!history.empty() && juce::String(history.back().content).contains("FANTASIO-1946"));
            ask("What was the secret code of the studio?", "FANTASIO-1946");
        }

        // The query is aborted during the generation and sent again: the context memory contains the partial response
        // after the end of the history, so the context memory is rolled back before the generation prompt (with the
        // checkpoint for the hybrid models)
        beginTest("Abort");
        {
            // The response must be long enough to abort the query during the generation
            auto const prompt = juce::String("Write a detailed paragraph of about 200 words that describes the studio and that includes its secret code. Do not use tools.");
            auto const numMessages = agent.getHistory().size();
            // The temporary response of the previous query is only cleared when the query starts
            auto const previousResponse = agent.getTemporaryResponse();
            std::atomic<bool> running{true};
            std::thread stopper([&]()
                                {
                                    while(running.load())
                                    {
                                        // The response must exceed the bounded rollback of the recurrent states
                                        auto const response = agent.getTemporaryResponse();
                                        if(response != previousResponse && response.length() > 300)
                                        {
                                            agent.setShouldQuit(true);
                                            return;
                                        }
                                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                    }
                                });
            auto const abortResult = agent.sendQuery(prompt, true);
            running.store(false);
            stopper.join();
            agent.setShouldQuit(false);
            expect(abortResult.failed(), "The query has not been aborted");
            expectEquals(agent.getHistory().size(), numMessages);

            auto const result = agent.sendQuery(prompt, true);
            auto const history = agent.getHistory();
            auto const answer = history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str());
            logMessage("After abort -> " + answer + " (context usage " + juce::String(agent.getContextCapacityUsage(), 3) + ")");
            expect(result.wasOk(), result.getErrorMessage());
            expect(answer.contains("FANTASIO-1946"), answer);
        }

        // A media file of the history has been deleted: the media is replaced by a text and the conversation continues
        if(Application::Neuralyzer::AgentLocal::getProjectorForModel(info.modelFile) != juce::File{})
        {
            beginTest("Missing Media");
            expect(imageFile.getFile().deleteFile());
            ask("What is the name of the Partiels document I am working on?", "Zorglub");
        }

        auto const createInventory = [](int numItems)
        {
            juce::String inventory = "Here is the inventory of my studio:";
            for(auto i = 1; i <= numItems; ++i)
            {
                inventory << " item " << i << " is a box of " << (i * 7) % 100 << " cables;";
            }
            return inventory;
        };

        // The history occupies a significant part of the context so the second query summarizes the session
        beginTest("Summary");
        // The context leaves enough space for the summary (the model does not always respect its maximum length)
        info.contextSize = 19456;
        expect(agent.initializeModel(info).wasOk());
        expect(agent.startSession().wasOk());
        ask("The Partiels document I am working on is named Zorglub. " + createInventory(400) + " Just acknowledge.", {}, false);
        expect(agent.getContextCapacityUsage() > 0.8f, juce::String(agent.getContextCapacityUsage()));
        // The content of the summary depends on the model so the answer is only logged
        ask("What is the name of the Partiels document I am working on?", {}, false);
        auto history = agent.getHistory();
        expect(history.size() == 5_z && juce::String(history.at(1_z).content).startsWith("Summarize our conversation"), juce::String(history.size()));
        if(history.size() == 5_z)
        {
            logMessage("Summary: " + juce::String::fromUTF8(history.at(2_z).content.c_str()));
        }

        // The system prompt occupies most of the context so the session is not summarized and the user is invited to
        // reset the history when the context is full
        beginTest("Context Full");
        info.contextSize = 13824;
        expect(agent.initializeModel(info).wasOk());
        expect(agent.startSession().wasOk());
        ask("The Partiels document I am working on is named Zorglub.", {}, false);
        expect(agent.getContextCapacityUsage() > 0.8f, juce::String(agent.getContextCapacityUsage()));
        // The model may refuse to answer without reasoning so the answer is only logged
        ask("What is the name of the Partiels document I am working on?", {}, false);
        history = agent.getHistory();
        expect(history.size() == 5_z && juce::String(history.at(1_z).content).startsWith("The Partiels document"), juce::String(history.size()));
        auto const result = agent.sendQuery(createInventory(400), false);
        logMessage("Context full: " + result.getErrorMessage());
        expect(result.failed() && result.getErrorMessage() == Application::Neuralyzer::EngineLocal::getContextFullMessage(), result.getErrorMessage());
        expect(agent.getHistory().size() == 5_z, juce::String(agent.getHistory().size()));
    }
};

static AgentLocalTest agentLocalTest;

ANALYSE_FILE_END
