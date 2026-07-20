#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationInstance.h"
#include <AnlNeuralyzerData.h>
#include <TestResultsData.h>
#include <mtmd-helper.h>

static void to_json(nlohmann::json& json, common_chat_msg const& message)
{
    json["role"] = message.role;
    json["content"] = message.content;
    json["reasoning_content"] = message.reasoning_content;
    json["tool_name"] = message.tool_name;
    json["tool_call_id"] = message.tool_call_id;

    json["content_parts"] = nlohmann::json::array();
    for(auto const& contentPart : message.content_parts)
    {
        nlohmann::json contentPartJson;
        contentPartJson["type"] = contentPart.type;
        contentPartJson["text"] = contentPart.text;
        json["content_parts"].push_back(std::move(contentPartJson));
    }

    json["tool_calls"] = nlohmann::json::array();
    for(auto const& toolCall : message.tool_calls)
    {
        nlohmann::json toolCallJson;
        toolCallJson["name"] = toolCall.name;
        toolCallJson["arguments"] = toolCall.arguments;
        toolCallJson["id"] = toolCall.id;
        json["tool_calls"].push_back(std::move(toolCallJson));
    }
}

static void from_json(nlohmann::json const& json, common_chat_msg& message)
{
    message = common_chat_msg{};
    message.role = json.at("role").get<std::string>();
    message.content = json.value("content", std::string{});
    message.reasoning_content = json.value("reasoning_content", std::string{});
    message.tool_name = json.value("tool_name", std::string{});
    message.tool_call_id = json.value("tool_call_id", std::string{});

    if(json.contains("content_parts") && json.at("content_parts").is_array())
    {
        for(auto const& contentPartJson : json.at("content_parts"))
        {
            common_chat_msg_content_part contentPart;
            contentPart.type = contentPartJson.value("type", std::string{});
            contentPart.text = contentPartJson.value("text", std::string{});
            message.content_parts.push_back(std::move(contentPart));
        }
    }

    if(json.contains("tool_calls") && json.at("tool_calls").is_array())
    {
        for(auto const& toolCallJson : json.at("tool_calls"))
        {
            common_chat_tool_call toolCall;
            toolCall.name = toolCallJson.value("name", std::string{});
            toolCall.arguments = toolCallJson.value("arguments", std::string{});
            toolCall.id = toolCallJson.value("id", std::string{});
            message.tool_calls.push_back(std::move(toolCall));
        }
    }
}

static juce::Result addMessage(common_chat_params& params, common_chat_templates_inputs& currentChatInputs, std::string const& role, std::string query, common_chat_templates const* chatTemplates, std::string instructions, bool const allowTools)
{
    common_chat_templates_inputs chatInputs;
    std::optional<common_chat_msg> preSystemMsg;
    chatInputs.add_generation_prompt = true;
    // REQUIRED forces the model to call a tool on this turn (local chatInputs copy only,
    // does not affect mChatInputs). The end-of-turn value written to mChatInputs below
    // is AUTO, so these two assignments intentionally operate on different objects.
    chatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_REQUIRED : COMMON_CHAT_TOOL_CHOICE_NONE;
    {
        chatInputs.use_jinja = currentChatInputs.use_jinja;
        chatInputs.reasoning_format = currentChatInputs.reasoning_format;
        chatInputs.enable_thinking = currentChatInputs.enable_thinking;
        chatInputs.parallel_tool_calls = currentChatInputs.parallel_tool_calls;
        chatInputs.add_bos = currentChatInputs.add_bos;
        chatInputs.add_eos = currentChatInputs.add_eos;
        if(currentChatInputs.messages.empty())
        {
            // If there are no messages in the history, we add the system prompt from
            // the instructions before applying templates. If there are already messages
            // in the history, we assume the system prompt is already included and do
            // not add it again. Tools are only included on the first message to avoid
            // wasting context tokens on subsequent iterations.
            chatInputs.tools = currentChatInputs.tools;
            common_chat_msg systemMsg;
            systemMsg.role = "system";
            systemMsg.content = std::move(instructions);
            currentChatInputs.messages.push_back(systemMsg);
            chatInputs.messages.push_back(std::move(systemMsg));
        }
    }

    common_chat_msg userMsg;
    userMsg.role = role;
    userMsg.content = std::move(query);
    currentChatInputs.messages.push_back(userMsg);
    userMsg.role = "user";
    chatInputs.messages.push_back(std::move(userMsg));

    try
    {
        params = common_chat_templates_apply(chatTemplates, chatInputs);
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to apply chat templates: ERROR").replace("ERROR", juce::String::fromUTF8(e.what())));
    }
    catch(...)
    {
        return juce::Result::fail(juce::translate("Failed to apply chat templates: ERROR").replace("ERROR", "Unknown error"));
    }
    return juce::Result::ok();
}

static bool isMediaAudioFileForInjection(juce::File const& file)
{
    return file.hasFileExtension("wav;flac;riff");
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

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_WARN)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::CharPointer_UTF8(text));
    }
}

juce::File Application::Neuralyzer::AgentLocal::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Models");
}

juce::File Application::Neuralyzer::AgentLocal::getDefaultProjectorDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Projectors");
}

std::set<juce::File> Application::Neuralyzer::AgentLocal::getAvailableModels()
{
    std::set<juce::File> models;
    auto const addFilesFromDirectory = [&](juce::File const& root)
    {
        auto const directory = resolveDirectory(root).getChildFile("Models");
        auto const listedModels = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.gguf");
        for(auto const& model : listedModels)
        {
            models.insert(model);
        }
    };
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::commonApplicationDataDirectory));
    return models;
}

std::set<juce::File> Application::Neuralyzer::AgentLocal::getAvailableProjectors()
{
    std::set<juce::File> models;
    auto const addFilesFromDirectory = [&](juce::File const& root)
    {
        auto const directory = resolveDirectory(root).getChildFile("Projectors");
        auto const listedModels = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.gguf");
        for(auto const& model : listedModels)
        {
            models.insert(model);
        }
    };
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory));
    addFilesFromDirectory(juce::File::getSpecialLocation(juce::File::SpecialLocationType::commonApplicationDataDirectory));
    return models;
}

std::vector<Application::Neuralyzer::AgentLocal::ModelBundle> Application::Neuralyzer::AgentLocal::getDefaultModelBundles()
{
    static auto const modelURL = juce::String("https://huggingface.co/unsloth/MODELNAME-GGUF/resolve/main/MODELNAME-UD-Q4_K_M.gguf");
    static auto const modelFile = getDefaultModelDirectory().getChildFile("MODELNAME-UD-Q4_K_M.gguf").getFullPathName();
    static auto const projectordURL = juce::String("https://huggingface.co/unsloth/MODELNAME-GGUF/resolve/main/mmproj-BF16.gguf");
    static auto const projectordFile = getDefaultProjectorDirectory().getChildFile("MODELNAME-UD-mmproj.gguf").getFullPathName();
    static auto constexpr modelNames = {"Qwen3.5-9B", "Qwen3.6-27B", "Qwen3.6-35B-A3B"};

    static auto const bundles = [&]()
    {
        std::vector<ModelBundle> bdls;
        for(auto const& name : modelNames)
        {

            // clang-format off
            bdls.push_back({
                  juce::String(name) + "-UD"
                , juce::URL(modelURL.replace("MODELNAME", name))
                , juce::File(modelFile.replace("MODELNAME", name))
                , juce::URL(projectordURL.replace("MODELNAME", name))
                , juce::File(projectordFile.replace("MODELNAME", name))
            });
            // clang-format on
        }
        return bdls;
    }();
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
                                         .withTitle(juce::translate("Failed to download the model NAME").replace("NAME", file.getFileNameWithoutExtension()))
                                         .withMessage(result.getErrorMessage())
                                         .withButton(juce::translate("Ok"));
                juce::AlertWindow::showAsync(options, nullptr);
            }
            return;
        }

        auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
        auto const otherFile = file == bundle.modelFile ? bundle.projectorFile : bundle.modelFile;
        // Ensure both the model and the projector are available
        if(!otherFile.existsAsFile() || downloader.isDownloading(otherFile))
        {
            return;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Model Downloaded"))
                                 .withMessage(juce::translate("The model NAME and its projector have been successfully downloaded. You can now select them in the model and projector lists. Would you like to open the Neuralyzer settings panel to select it now?").replace("NAME", bundle.name))
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
    };

    auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
    if(!bundle.modelFile.existsAsFile())
    {
        downloader.start(bundle.modelFile, bundle.projectorUrl, callback);
    }
    if(!bundle.projectorFile.existsAsFile())
    {
        downloader.start(bundle.projectorFile, bundle.projectorUrl, callback);
    }
}

void Application::Neuralyzer::AgentLocal::downloadDefaultModelIfNecessary()
{
    if(!getAvailableModels().empty() && !getAvailableProjectors().empty())
    {
        return;
    }
    downloadModelBundle(getDefaultModelBundle(), false);
}

Application::Neuralyzer::AgentLocal::AgentLocal(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine)
: mMcpDispatcher(mcpDispatcher)
, mRagEngine(ragEngine)
{
    mMcpMethods.readFilesFn = std::bind(&AgentLocal::readFiles, this, std::placeholders::_1);
    mMcpMethods.searchDocsFn = std::bind(&AgentLocal::searchDocs, this, std::placeholders::_1, std::placeholders::_2);
    mMcpMethods.loadDocsFn = std::bind(&AgentLocal::loadDocs, this, std::placeholders::_1);
}

Application::Neuralyzer::AgentLocal::~AgentLocal()
{
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
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

juce::Result Application::Neuralyzer::AgentLocal::initializeModel(ModelInfo const& info)
{
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::AgentLocal", "Initialize...");

    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates = nullptr;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs = common_chat_templates_inputs{};
    }
    notifyStateChanged();
    mContextMemoryUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }

    if(info.modelFile == juce::File())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The model file is not set.");
        return juce::Result::fail(juce::translate("The model file is not set."));
    }
    if(!info.modelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The model file does not exist: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("The model file does not exist: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
    }

    // Configure common_params for model and context initialization
    common_params params;
    params.model.path = info.modelFile.getFullPathName().toStdString();
    auto const templateFile = info.modelFile.withFileExtension(".jinja");
    params.chat_template = templateFile.loadFileAsString().toStdString();
    params.use_jinja = !params.chat_template.empty();
    params.n_ctx = std::min(info.contextSize.value_or(params.n_ctx), maxContextSize);
    params.n_batch = info.batchSize.value_or(params.n_batch);
    params.sampling.min_p = info.minP.value_or(params.sampling.min_p);
    params.sampling.temp = info.temperature.value_or(params.sampling.temp);
    params.sampling.top_p = info.topP.value_or(params.sampling.top_p);
    params.sampling.top_k = info.topK.value_or(params.sampling.top_k);
    params.sampling.penalty_present = info.presencePenalty.value_or(params.sampling.penalty_present);
    params.sampling.penalty_repeat = info.repetitionPenalty.value_or(params.sampling.penalty_repeat);
    params.load_progress_callback_user_data = static_cast<void*>(this);
    params.load_progress_callback = [](float, void* data) -> bool
    {
        return !reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
    };

    // Initialize model, context and sampler as a single lifetime-managed object.
    mInitResult = common_init_from_params(params);
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }
    if(mInitResult == nullptr || mInitResult->model() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to load model from: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
    }

    llama_set_abort_callback(mInitResult->context(), [](void* data)
                             {
                                 return reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
                             },
                             static_cast<void*>(this));

    auto const* context = mInitResult->context();
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    auto const batchCapacity = static_cast<size_t>(llama_n_batch(context));

    MiscDebug("Application::Neuralyzer::AgentLocal", "Model, context, and sampler loaded successfully with context size: " + juce::String(ctxCapacity) + " and batch size: " + juce::String(batchCapacity));

    // Initialize chat templates with Jinja support
    if(!params.chat_template.empty() && !common_chat_verify_template(params.chat_template, true))
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The chat template is not supported: " + templateFile.getFullPathName());
        return juce::Result::fail(juce::translate("The chat template is not supported: FLNAME").replace("FLNAME", templateFile.getFullPathName()));
    }
    try
    {
        mChatTemplates = common_chat_templates_init(mInitResult->model(), params.chat_template);
    }
    catch(...)
    {
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to initialize chat templates: fallback to default.");
        mChatTemplates = common_chat_templates_init(mInitResult->model(), std::string{});
    }
    if(mChatTemplates == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to initialize chat templates.");
        return juce::Result::fail(juce::translate("Failed to initialize chat templates"));
    }

    auto mtmdFile = info.projectionFile;
    if(mtmdFile != juce::File())
    {
        if(!mtmdFile.existsAsFile())
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "The projection file does not exist: " + mtmdFile.getFullPathName());
            return juce::Result::fail(juce::translate("The projection file does not exist: FLNAME").replace("FLNAME", mtmdFile.getFullPathName()));
        }

        auto mtmdParams = mtmd_context_params_default();
        mtmdParams.progress_callback_user_data = static_cast<void*>(this);
        mtmdParams.progress_callback = [](float, void* data) -> bool
        {
            return !reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
        };

        try
        {
            mMtmdContext.reset(mtmd_init_from_file(mtmdFile.getFullPathName().toRawUTF8(), mInitResult->model(), mtmdParams));
            if(mMtmdContext != nullptr)
            {
                MiscDebug("Application::Neuralyzer::AgentLocal", "Multimodal context initialized successfully from projection file: " + mtmdFile.getFullPathName());
            }
            else
            {
                return juce::Result::fail(juce::translate("Failed to initialize multimodal context from projection file: FLNAME").replace("FLNAME", mtmdFile.getFullPathName()));
            }
        }
        catch(...)
        {
            MiscWeakAssert(false);
            mMtmdContext.reset();
            return juce::Result::fail(juce::translate("Failed to initialize multimodal context from projection file: FLNAME").replace("FLNAME", mtmdFile.getFullPathName()));
        }
    }

    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    // Build tools from MCP
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.use_jinja = true;
        mChatInputs.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
        mChatInputs.enable_thinking = false;
        mChatInputs.parallel_tool_calls = true;
        auto const* vocab = llama_model_get_vocab(mInitResult->model());
        mChatInputs.add_bos = llama_vocab_get_add_bos(vocab);
        mChatInputs.add_eos = llama_vocab_get_add_eos(vocab);
        mChatInputs.tools = mMcpDispatcher.getToolList(mMcpMethods);
    }

    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", "Successfully initialized model: " + info.modelFile.getFullPathName());
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo.modelFile = info.modelFile;
        mModelInfo.projectionFile = info.projectionFile;
        mModelInfo.contextSize = ctxCapacity;
        mModelInfo.batchSize = batchCapacity;
        mModelInfo.minP = params.sampling.min_p;
        mModelInfo.temperature = params.sampling.temp;
        mModelInfo.topP = params.sampling.top_p;
        mModelInfo.topK = params.sampling.top_k;
        mModelInfo.presencePenalty = params.sampling.penalty_present;
        mModelInfo.repetitionPenalty = params.sampling.penalty_repeat;
    }
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::resetModel()
{
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates = nullptr;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs = common_chat_templates_inputs{};
    }
    notifyStateChanged();
    mContextMemoryUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::addMedia(juce::File const& file)
{
    if(file == juce::File{} || !file.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The media file does not exist: " + file.getFullPathName());
        return juce::Result::fail(juce::translate("The media file does not exist: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }

    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    if(context == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(mMtmdContext == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The current model does not expose a multimodal context.");
        return juce::Result::fail(juce::translate("The current model does not support media input."));
    }
    auto const path = file.getFullPathName().toStdString();
    auto const wrapper = mtmd_helper_bitmap_init_from_file(mMtmdContext.get(), path.c_str(), false);
    if(wrapper.bitmap == nullptr)
    {
        return juce::Result::fail(juce::translate("Failed to load media file: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }
    mtmd::bitmap_ptr bitmap(wrapper.bitmap);
    if(wrapper.video_ctx != nullptr)
    {
        mtmd_helper_video_free(wrapper.video_ctx);
        return juce::Result::fail(juce::translate("Video media is not supported: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }

    auto chatInput = [this]()
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        return mChatInputs;
    }();
    common_chat_params params;
    auto const addResult = addMessage(params, chatInput, "media", std::string(mtmd_default_marker()) + path, mChatTemplates.get(), mMcpDispatcher.getInstructions(), false);
    if(addResult.failed())
    {
        return addResult;
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", params.prompt);
    mtmd_input_text text;
    text.text = params.prompt.c_str();
    text.text_len = params.prompt.size();
    text.add_special = chatInput.messages.size() > 1_z;
    text.parse_special = true;

    auto const* bitmapPtr = bitmap.get();
    mtmd::input_chunks chunks(mtmd_input_chunks_init());
    if(mtmd_tokenize(mMtmdContext.get(), chunks.ptr.get(), &text, &bitmapPtr, 1) != 0)
    {
        return juce::Result::fail(juce::translate("Failed to tokenize media prompt."));
    }

    auto const batchSize = static_cast<int32_t>(llama_n_batch(context));
    auto const nPast = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    auto newNPast = nPast;
    if(mtmd_helper_eval_chunks(mMtmdContext.get(), context, chunks.ptr.get(), nPast, 0, batchSize, false, &newNPast) != 0)
    {
        return juce::Result::fail(juce::translate("Failed to decode media prompt."));
    }

    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs = chatInput;
    }

    updateContextMemoryUsage();
    notifyStateChanged();
    return juce::Result::ok();
}

std::tuple<juce::Result, std::string, common_chat_params> Application::Neuralyzer::AgentLocal::performMessage(std::string const& role, std::string query, bool allowTools)
{
    auto const createResults = [](juce::Result result, std::string message = {}, common_chat_params params = {})
    {
        return std::make_tuple(std::move(result), std::move(message), std::move(params));
    };

    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    auto* sampler = mInitResult != nullptr ? mInitResult->sampler(0) : nullptr;
    MiscStrongAssert(context != nullptr && sampler != nullptr);

    MiscDebug("Application::Neuralyzer::AgentLocal", role + ": " + query);

    auto chatInput = [this]()
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        return mChatInputs;
    }();
    common_chat_params params;
    auto const addResult = addMessage(params, chatInput, role, std::move(query), mChatTemplates.get(), mMcpDispatcher.getInstructions(), allowTools);
    if(addResult.failed())
    {
        return createResults(std::move(addResult));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", "Prompt: " + juce::String(params.prompt).replace("\n", "\\n"));
    auto tokens = common_tokenize(context, params.prompt, true, true);
    // Store the formatted prompt for delta extraction

    // Check if prompt tokens will exceed context capacity
    auto const spaceResults = manageContentMemory(tokens.size());
    if(spaceResults.failed())
    {
        return createResults(spaceResults);
    }
    if(mShouldQuit.load())
    {
        return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
    }

    auto const batchSize = static_cast<size_t>(llama_n_batch(context));

    // Send the prompt tokens into the model in batches
    // Decode the prompt in chunks so prompts larger than n_batch still work.
    auto position = 0_z;
    while(position < tokens.size())
    {
        auto const size = std::min(batchSize, tokens.size() - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
        auto const ret = llama_decode(context, batch);
        if(mShouldQuit.load())
        {
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
        }
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")));
        }
        position += size;
        updateContextMemoryUsage();
    }
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs = std::move(chatInput);
    }
    notifyStateChanged();

    // Get the assistant response with streaming support.
    common_chat_msg msg;
    msg.role = "assistant";
    while(true)
    {
        // Sample the next token using common_sampler
        auto new_token_id = common_sampler_sample(sampler, context, -1);
        common_sampler_accept(sampler, new_token_id, true);
        auto const* vocab = llama_model_get_vocab(llama_get_model(context));
        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(vocab, new_token_id))
        {
            break;
        }

        auto const piece = common_token_to_piece(vocab, new_token_id, true);
        msg.content += piece;
        {
            // Write to mTempResponse only — do NOT call notifyStateChanged() here.
            // The UI polls getTemporaryResponse() via a timer; calling notifyStateChanged()
            // per token would flood the message queue with change notifications.
            std::unique_lock<std::mutex> lock(mTemporaryMutex);
            mTempResponse = msg.content;
        }

        auto const batch = llama_batch_get_one(&new_token_id, 1);
        auto const ret = llama_decode(context, batch);
        if(mShouldQuit.load())
        {
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
        }
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")), msg.content);
        }
    }
    MiscDebug("Application::Neuralyzer::AgentLocal", msg.content);

    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages.push_back(msg);
        mChatInputs.add_generation_prompt = false;
        // AUTO is stored in mChatInputs (persistent history) so that future calls to
        // common_chat_templates_apply use a relaxed policy, while the entry tool_choice
        // above (REQUIRED) only applied to the local chatInputs copy used for this turn.
        mChatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_AUTO : COMMON_CHAT_TOOL_CHOICE_NONE;
        params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
        notifyStateChanged();
    }

    updateContextMemoryUsage();
    return createResults(juce::Result::ok(), msg.content, std::move(params));
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
        juce::File const file(filePath);
        if(!file.existsAsFile())
        {
            message += "The file does not exist.";
            response["isError"] = true;
        }
        else if(!file.hasReadAccess())
        {
            message += "The file does have read access.";
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
    content["text"] = message;
    response["content"].push_back(content);
    return response;
}

nlohmann::json Application::Neuralyzer::AgentLocal::searchDocs(std::string const& query, size_t maxNumResources)
{
    nlohmann::json response;
    response["isError"] = false;
    response["content"] = nlohmann::json::array();

    auto const resources = mRagEngine.computeResources(juce::String::fromUTF8(query.c_str()), maxNumResources);
    std::string message;
    if(resources.empty())
    {
        message += "No relevant document was found.";
    }
    else
    {
        message += "Relevant documents:\n";
        for(auto index = 0_z; index < resources.size(); ++index)
        {
            auto const resource = resources.at(index);
            message += "\n";
            message += "[" + std::to_string(index + 1) + "]" + "\n";
            message += "ID: " + resource.id.toStdString() + "\n";
            message += "Title: " + resource.document.toStdString() + "\n";
            message += "Section: " + resource.section.toStdString() + "\n";
            message += "Score: " + std::to_string(resource.score) + "\n";
        }
        message += "\nRetrieve the contents of the documentation using the load_docs tool with the IDs of the documents.";
    }

    nlohmann::json content;
    content["text"] = message;
    response["content"].push_back(content);
    return response;
}

nlohmann::json Application::Neuralyzer::AgentLocal::loadDocs(std::vector<std::string> const& ids)
{
    nlohmann::json response;
    response["isError"] = false;
    response["content"] = nlohmann::json::array();

    auto const resources = mRagEngine.getResources(std::vector<juce::String>{ids.cbegin(), ids.cend()});
    std::string message;
    if(resources.empty())
    {
        response["isError"] = true;
        message += "No document was found for the given ids.";
    }
    else
    {
        message += "Relevant documents:\n";
        for(auto const& resource : resources)
        {
            message += "\n--------\n";
            message += "ID: " + resource.id.toStdString() + "\n";
            message += "Title: " + resource.document.toStdString() + "\n";
            message += "Section: " + resource.section.toStdString() + "\n";
            message += "Content:\n" + resource.content.toStdString() + "\n";
        }
        message += "\nSearch for complementary documentation using the search_docs tool.";
    }

    nlohmann::json content;
    content["text"] = message;
    response["content"].push_back(content);
    return response;
}

juce::Result Application::Neuralyzer::AgentLocal::performQuery(juce::String const& prompt, bool allowTools)
{
    {
        std::unique_lock<std::mutex> lock(mTemporaryMutex);
        mTempResponse.clear();
    }

    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Agent loop: keep calling tools until Llama provides a final answer
    auto currentQuery = prompt.toStdString();
    auto numErrors = 0_z;
    static auto constexpr maxErrors = 5_z;      // Abort if too perform errors
    static auto constexpr maxIterations = 25_z; // Prevent infinite loops

    auto const tempSessionFile = getTempSessionFile("localagent.ptldoc");
    auto const createError = [&](juce::String const& errorMessage, bool restoreSession) -> juce::Result
    {
        // Add an error message to the history so the user understands what happened.
        if(restoreSession)
        {
            // Restore the session so the user can then try a different query or reset to recover from this situation.
            loadSession(tempSessionFile);
        }
        MiscDebug("Application::Neuralyzer::AgentLocal", errorMessage);
        return juce::Result::fail(errorMessage);
    };

    auto const numRequiredSpace = static_cast<size_t>(llama_n_ctx(mInitResult->context()) / 4);
    auto const* role = "user";
    for(auto iteration = 0_z; iteration < maxIterations && mShouldQuit.load() == false; ++iteration)
    {
        // Save a backup of the current session to allow recovery in case of a crash during tool calls.
        // We save it to the temp directory to avoid cluttering the user's documents with backup files,
        // and we use a fixed file name since we only need to keep the most recent backup.
        saveSession(tempSessionFile);

        // Manage context size before each iteration to prevent overflow
        auto ctxResult = manageContentMemory(numRequiredSpace);
        if(ctxResult.failed())
        {
            return createError(juce::translate("Context memory exceeded: ERROR").replace("ERROR", ctxResult.getErrorMessage()), true);
        }

        // Perform a message exchange with the model, get the assistant response
        // and any chat parameters (e.g. tools to call).
        auto const [performResult, assistantMsg, chatParams] = performMessage(role, currentQuery, allowTools);
        if(performResult.failed() || assistantMsg.empty())
        {
            auto const errorMessage = performResult.failed() ? performResult.getErrorMessage() : juce::translate("Unknown error");
            return createError(juce::translate("Failed to get a response from the model: ERROR").replace("ERROR", errorMessage), true);
        }
        else
        {
            role = "tool";
            // The model successfully generated a response, we parse the response for
            // tool calls and either call the tools or return the final answer.
            common_chat_parser_params parserParams(chatParams);
            if(!chatParams.parser.empty())
            {
                parserParams.parser.load(chatParams.parser);
            }
            auto const [parsingError, toolCalls] = [&, inputMsg = assistantMsg]()
            {
                try
                {
                    return std::make_tuple(std::string{}, common_chat_parse(inputMsg, false, parserParams).tool_calls);
                }
                catch(std::exception const& e)
                {
                    MiscDebug("Application::Neuralyzer::AgentLocal", "Exception during tool call parsing: " + juce::String::fromUTF8(e.what()));
                    return std::make_tuple(std::string("Exception during tool call parsing: ") + juce::String::fromUTF8(e.what()).toStdString(), std::vector<common_chat_tool_call>{});
                }
                catch(...)
                {
                    MiscDebug("Application::Neuralyzer::AgentLocal", "Unknown exception during tool call parsing");
                    return std::make_tuple(std::string("Exception during tool call parsing: Unknown"), std::vector<common_chat_tool_call>{});
                }
            }();
            if(!parsingError.empty())
            {
                // The tool parsing failed, we provide the parsing error as feedback
                // to the model and do not call any tools.
                ++numErrors;
                currentQuery = parsingError + "\n\n" + getErrorInstuction(maxErrors, numErrors);
                allowTools = allowTools && numErrors < maxErrors - 1;
                if(numErrors >= maxErrors)
                {
                    // The tool call failed for too many consecutive iterations,
                    // we abort the entire query.
                    return createError(juce::translate("Tool call failed: ERROR").replace("warning", parsingError), false);
                }
            }
            else
            {
                currentQuery = callTools(mMcpDispatcher, mMcpMethods, toolCalls, maxErrors, numErrors);
                if(numErrors >= maxErrors)
                {
                    // The tool call failed for too many consecutive iterations,
                    // we abort the entire query.
                    return createError(juce::translate("Tool call failed: ERROR").replace("warning", currentQuery), false);
                }
                else if(currentQuery.empty())
                {
                    // The tool call succeeded and there are no tool responses,
                    // we use the last assistant message as the final answer.
                    return juce::Result::ok();
                }
                allowTools = allowTools && numErrors < maxErrors - 1;
            }
        }
    }
    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted"), true);
    }

    // Max iterations reached - return the last response
    MiscDebug("Application::Neuralyzer::AgentLocal", "Maximum iterations reached without reaching a final answer.");
    return createError(juce::translate("Maximum iterations reached without reaching a final answer."), false);
}

juce::Result Application::Neuralyzer::AgentLocal::sendQuery(juce::String const& prompt, juce::StringArray const& files)
{
    {
        std::unique_lock<std::mutex> lock(mTemporaryMutex);
        mTempResponse.clear();
    }

    juce::String fullQuery;
    // Adds the file paths to the query
    if(!files.isEmpty())
    {
        fullQuery << "Attached files:\n";
        for(auto const& file : files)
        {
            fullQuery << "- " << file << "\n";
        }
        fullQuery << "\n";
    }

    // Adds the user request to the query
    fullQuery << "User request:\n" + prompt;
    return performQuery(fullQuery, true);
}

juce::Result Application::Neuralyzer::AgentLocal::startSession()
{
    return clearContextMemory();
}

juce::Result Application::Neuralyzer::AgentLocal::loadSession(juce::File const& sessionFile)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
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

    auto const root = [&]()
    {
        try
        {
            return nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        }
        catch(...)
        {
            return nlohmann::json{};
        }
    }();
    auto messages = [&]() -> std::vector<common_chat_msg>
    {
        try
        {
            return root.value("messages", std::vector<common_chat_msg>{});
        }
        catch(...)
        {
            return {};
        }
    }();
    if(messages.size() <= kNumInitMessages)
    {
        return juce::Result::fail(juce::translate("No messages found in session file."));
    }

    auto const version = [&]() -> std::string
    {
        try
        {
            return root.value("version", std::string{});
        }
        catch(...)
        {
            return {};
        }
    }();

    auto const shouldUpdate = version != mMcpDispatcher.getUuid();
    if(shouldUpdate && !messages.empty())
    {
        messages.front().content = mMcpDispatcher.getInstructions();
    }

    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages = std::move(messages);
    }
    // Clear the context memory
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);

    if(shouldUpdate)
    {
        return decodeContextMemory();
    }
    auto const contextFile = sessionFile.withFileExtension(".ctx");
    if(contextFile.getSize() <= 0)
    {
        return decodeContextMemory();
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    size_t nloaded = 0;
    std::vector<llama_token> restoredTokens(static_cast<size_t>(llama_n_ctx(context)));
    if(!llama_state_load_file(context, contextFile.getFullPathName().toRawUTF8(), restoredTokens.data(), restoredTokens.size(), &nloaded) || nloaded == 0)
    {
        return decodeContextMemory();
    }
    updateContextMemoryUsage();
    notifyStateChanged();
    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + contextFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::saveSession(juce::File const& sessionFile)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file provided."));
    }
    auto const contextFile = sessionFile.withFileExtension(".ctx");

    // Ensure directory exists
    if(!contextFile.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to create directory: " + contextFile.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", contextFile.getParentDirectory().getFullPathName()));
    }
    if(!sessionFile.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to create directory: " + sessionFile.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", sessionFile.getParentDirectory().getFullPathName()));
    }

    auto const messages = [this]()
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        return mChatInputs.messages;
    }();

    if(messages.size() <= kNumInitMessages)
    {
        return juce::Result::fail(juce::translate("No messages to save in session."));
    }

    nlohmann::json root;
    root["version"] = mMcpDispatcher.getUuid();
    root["messages"] = messages;

    if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to save state metadata file: " + sessionFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state metadata file: FLNAME").replace("FLNAME", sessionFile.getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    auto* context = mInitResult->context();
    common_chat_params stateParams;
    auto stateInputs = [this]()
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        return mChatInputs;
    }();

    stateInputs.add_generation_prompt = false;
    stateInputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_NONE;
    try
    {
        stateParams = common_chat_templates_apply(mChatTemplates.get(), stateInputs);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Failed to apply chat templates when saving session: ") + e.what());
        return juce::Result::fail(juce::translate("Failed to apply chat templates: ") + e.what());
    }

    auto const stateTokens = common_tokenize(context, stateParams.prompt, true, true);
    auto const saved = llama_state_save_file(context, contextFile.getFullPathName().toRawUTF8(), stateTokens.data(), stateTokens.size());
    if(!saved)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to save state to file: " + contextFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", contextFile.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Saved KV cache and message history to files: ") + contextFile.getFullPathName() + " " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::clearContextMemory()
{
    // Clear chat history and KV cache from system prompt onwards
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The model is not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages.clear();
    }
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);
    updateContextMemoryUsage();
    notifyStateChanged();
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::decodeContextMemory()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // No KV cache file: restore the message history and manually integrate it into
    // the context by decoding the full conversation prompt.
    auto chatInputs = [this]()
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        return mChatInputs;
    }();

    for(auto& message : chatInputs.messages)
    {
        if(message.role == "system")
        {
            message.content = mMcpDispatcher.getInstructions();
        }
        else if(message.role == "tool" || message.role == "media" || message.role == "warning")
        {
            // For the moment, the media are not restored
            message.role = "user";
        }
    }

    chatInputs.add_generation_prompt = false;
    chatInputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_REQUIRED;
    common_chat_params params;
    try
    {
        params = common_chat_templates_apply(mChatTemplates.get(), chatInputs);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Failed to apply chat templates when restoring session: ") + e.what());
        return juce::Result::fail(juce::translate("Failed to apply chat templates: ISSUE.").replace("ISSUE", e.what()));
    }

    auto* context = mInitResult->context();
    auto const batchSize = static_cast<int32_t>(llama_n_batch(context));
    size_t restoredCount = 0;
    auto tokens = common_tokenize(context, params.prompt, true, true);
    restoredCount = tokens.size();
    auto position = 0_z;
    while(position < tokens.size())
    {
        auto const size = std::min(static_cast<size_t>(batchSize), tokens.size() - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
        if(llama_decode(context, batch) != 0)
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to decode message history into context.");
            return juce::Result::fail(juce::translate("Failed to decode message history into context."));
        }
        position += size;
        updateContextMemoryUsage();
    }
    notifyStateChanged();
    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Restored ") + juce::String(static_cast<juce::int64>(restoredCount)));
    return juce::Result::ok();
}

Application::Neuralyzer::AgentLocal::History Application::Neuralyzer::AgentLocal::getHistory() const
{
    History history;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        history.reserve(mChatInputs.messages.size());
        for(auto const& message : mChatInputs.messages)
        {
            auto const role = magic_enum::enum_cast<MessageType>(message.role);
            auto const content = message.content.empty() ? message.render_content() : message.content;
            history.push_back({role.value_or(MessageType::assistant), juce::String(content)});
        }
    }
    return history;
}

juce::Result Application::Neuralyzer::AgentLocal::manageContentMemory(size_t minNumRequiredTokens)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::ok();
    }
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    if(ctxCapacity < minNumRequiredTokens)
    {
        return juce::Result::fail(juce::translate("The model's context capacity is smaller than the required tokens."));
    }
    auto const numCtxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
    if(ctxCapacity - numCtxUsed >= minNumRequiredTokens)
    {
        return juce::Result::ok();
    }
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        if(mChatInputs.messages.size() <= 1)
        {
            return juce::Result::fail(juce::translate("Context is nearing capacity and there are no messages to summarize or prune."));
        }
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Context nearing capacity."));

    // Guard: the summary itself needs room to be generated. The target length is
    // ctxCapacity/10 tokens. If less than that is available, performMessage would
    // immediately fail (or recurse), so fall back to a hard clear instead.
    auto const minSpaceForSummary = ctxCapacity / 10;
    if(ctxCapacity - numCtxUsed < minSpaceForSummary)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Context too full to summarize; falling back to hard clear.");
        return clearContextMemory();
    }

    {
        std::unique_lock<std::mutex> lock(mTemporaryMutex);
        mTempResponse = "Summarizing previous messages to manage context size...\n";
    }
    auto const summaryPrompt = juce::String("Summarize our conversation from the 2nd message up to last message in a concise way that preserves important details and context. This summary will replace the earlier messages in the context to free up space, so it should capture key points, decisions made, and any relevant information. The summary should be brief (MAXNUMTOKENS tokens max) but informative, allowing us to continue our discussion without losing important context.").replace("MAXNUMTOKENS", juce::String(ctxCapacity / 10));
    auto const summaryResult = performMessage("system", summaryPrompt.toStdString(), false);
    if(std::get<0>(summaryResult).failed())
    {
        return std::get<0>(summaryResult);
    }
    auto const clearResult = clearContextMemory();
    if(clearResult.failed())
    {
        return clearResult;
    }

    auto const followUpResult = performMessage("system", std::get<1>(summaryResult), false);
    return std::get<0>(followUpResult);
}

void Application::Neuralyzer::AgentLocal::updateContextMemoryUsage()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return;
    }
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    mContextMemoryUsage.store(static_cast<float>(static_cast<double>(ctxUsed) / static_cast<double>(ctxCapacity)));
}

float Application::Neuralyzer::AgentLocal::getContextCapacityUsage() const
{
    return mContextMemoryUsage.load();
}

juce::String Application::Neuralyzer::AgentLocal::getTemporaryResponse() const
{
    std::unique_lock<std::mutex> lock(mTemporaryMutex);
    auto response = mTempResponse;
    lock.unlock();
    return juce::String(response);
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
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    return mModelInfo;
}

class CharParserTest
: public juce::UnitTest
{
public:
    CharParserTest()
    : juce::UnitTest("Neuralyzer", "Application")
    {
    }

    ~CharParserTest() override = default;

    void runTest() override
    {
        auto const parseToolCall = [](const char* assistantMsg)
        {
            common_chat_tool chatTool;
            chatTool.name = "my_method";
            chatTool.description = "A multiproperties method";
            chatTool.parameters =
                "{\n"
                "  \"type\": \"object\",\n"
                "  \"properties\": {\n"
                "    \"identifier\": {\n"
                "      \"type\": \"string\",\n"
                "      \"description\": \"A string property\"\n"
                "    },\n"
                "    \"enabled\": {\n"
                "      \"type\": \"boolean\",\n"
                "      \"description\": \"A boolean property\"\n"
                "    },\n"
                "    \"count\": {\n"
                "      \"type\": \"integer\",\n"
                "      \"description\": \"A integer property\"\n"
                "    }\n"
                "  },\n"
                "  \"required\": [\n"
                "    \"identifier\"\n"
                "  ]\n"
                "}";

            common_chat_templates_inputs inputs;
            inputs.use_jinja = true;
            inputs.enable_thinking = false;
            inputs.parallel_tool_calls = true;
            inputs.add_generation_prompt = false;
            inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
            inputs.tools = {chatTool};
            common_chat_msg msg;
            msg.role = "system";
            msg.content = "System message content";
            inputs.messages.push_back(msg);
            msg.role = "user";
            msg.content = "User message content";
            inputs.messages.push_back(msg);
            msg.role = "assitant";
            msg.content = assistantMsg;
            inputs.messages.push_back(msg);

            auto tplt = common_chat_templates_init(nullptr, TestResultsData::NeuralyzerChatTemplate_jinja);
            common_chat_params chatParams = common_chat_templates_apply(tplt.get(), inputs);
            common_chat_parser_params parserParams(chatParams);
            if(!chatParams.parser.empty())
            {
                parserParams.parser.load(chatParams.parser);
            }
            common_chat_parse(assistantMsg, false, parserParams);
        };

        beginTest("Chat Parser Tools");
        {
            testCase("1 - Required Parameter Only", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
            testCase("2 - Reordered Parameters", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=enabled>\n"
                             "true\n"
                             "</parameter>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
            testCase("3 - Ordered Parameters", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "<parameter=enabled>\n"
                             "true\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
            testCase("4 - Reordered Parameters", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=count>\n"
                             "12\n"
                             "</parameter>\n"
                             "<parameter=enabled>\n"
                             "true\n"
                             "</parameter>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
            testCase("5 - Reordered Parameters", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "<parameter=count>\n"
                             "12\n"
                             "</parameter>\n"
                             "<parameter=enabled>\n"
                             "true\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
            testCase("6 - Upper case Boolean", [&]()
                     {
                         parseToolCall(
                             "<tool_call>\n"
                             "<function=my_method>\n"
                             "<parameter=identifier>\n"
                             "\"zaza\"\n"
                             "</parameter>\n"
                             "<parameter=enabled>\n"
                             "True\n"
                             "</parameter>\n"
                             "</function>\n"
                             "</tool_call>");
                     });
        }
    }
};

static CharParserTest charParserTest;

ANALYSE_FILE_END
