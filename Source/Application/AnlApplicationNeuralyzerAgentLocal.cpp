#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationInstance.h"
#include <AnlNeuralyzerData.h>
#include <TestResultsData.h>
#include <mtmd-helper.h>

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

static common_chat_params applyChatTemplates(common_chat_templates const* chatTplt, common_chat_templates_inputs const& inputs)
{
    try
    {
        return common_chat_templates_apply(chatTplt, inputs);
    }
    catch(std::exception& e)
    {
        MiscWeakAssert(false);
        throw juce::String("Failed to apply template: ERRSTR").replace("ERRSTR", e.what());
    }
}

static common_sampler_ptr createSampler(llama_model const* model, common_params_sampling const& samplingParams, common_chat_params const& chatParams)
{
    auto const* vocab = llama_model_get_vocab(model);
    try
    {
        auto params = samplingParams;
        if(!chatParams.grammar.empty())
        {
            params.grammar = {COMMON_GRAMMAR_TYPE_TOOL_CALLS, chatParams.grammar};
            params.grammar_lazy = chatParams.grammar_lazy;
            params.grammar_triggers = chatParams.grammar_triggers;
            for(auto const& preservedToken : chatParams.preserved_tokens)
            {
                auto const tokenIds = common_tokenize(vocab, preservedToken, false, true);
                if(tokenIds.size() == 1_z)
                {
                    params.preserved_tokens.insert(tokenIds.front());
                }
            }
            for(auto& trigger : params.grammar_triggers)
            {
                if(trigger.type == COMMON_GRAMMAR_TRIGGER_TYPE_WORD)
                {
                    auto const tokenIds = common_tokenize(vocab, trigger.value, false, true);
                    if(tokenIds.size() == 1_z && params.preserved_tokens.find(tokenIds.front()) != params.preserved_tokens.end())
                    {
                        trigger.type = COMMON_GRAMMAR_TRIGGER_TYPE_TOKEN;
                        trigger.token = tokenIds.front();
                    }
                }
            }
            params.generation_prompt = chatParams.generation_prompt;
        }
        return common_sampler_ptr(common_sampler_init(model, params));
    }
    catch(std::exception const& e)
    {
        throw juce::translate("Failed to initialize response sampler: ERRSTR").replace("ERRSTR", juce::String::fromUTF8(e.what()));
    }
}

static void injectMessages(llama_context* context, mtmd_context* mtmd, std::vector<common_chat_msg> const& messages, std::string const& prompt, std::function<bool(void)> callback)
{
    // Try to create bipmap for messages that contain a media
    std::vector<mtmd::bitmap_ptr> bitmaps;
    if(context == nullptr)
    {
        for(auto const& message : messages)
        {
            if(message.contains_media())
            {
                auto const wrapper = mtmd_helper_bitmap_init_from_file(mtmd, message.tool_name.c_str(), false, mtmd_helper_init_opt_default());
                if(wrapper.bitmap == nullptr)
                {
                    throw juce::translate("Failed to reload media file: FLNAME").replace("FLNAME", juce::String(message.tool_name));
                }
                if(wrapper.video_ctx != nullptr)
                {
                    mtmd_helper_video_free(wrapper.video_ctx);
                    throw juce::translate("Video media is not supported: FLNAME").replace("FLNAME", juce::String(message.tool_name));
                }
                bitmaps.emplace_back(wrapper.bitmap);
            }
        }
    }

    // Use MTMD if there are bitmaps, otherwise fallback to the default approach
    if(!bitmaps.empty())
    {
        std::vector<mtmd_bitmap const*> bitmapPtrs;
        for(auto const& bitmap : bitmaps)
        {
            bitmapPtrs.push_back(bitmap.get());
        }
        mtmd_input_text text;
        text.text = prompt.c_str();
        text.text_len = prompt.size();
        text.add_special = true;
        text.parse_special = true;

        mtmd::input_chunks chunks(mtmd_input_chunks_init());
        if(mtmd_tokenize(mtmd, chunks.ptr.get(), &text, bitmapPtrs.data(), bitmapPtrs.size()) != 0)
        {
            throw juce::translate("Failed to tokenize media prompt.");
        }

        auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
        auto const ctxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
        if(ctxCapacity - ctxUsed < mtmd_helper_get_n_tokens(chunks.ptr.get()))
        {
            throw juce::translate("The context capacity is not enough");
        }

        auto const batchSize = static_cast<int32_t>(llama_n_batch(context));
        auto const nPast = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
        llama_pos newNPast = nPast;
        if(mtmd_helper_eval_chunks(mtmd, context, chunks.ptr.get(), nPast, 0, batchSize, false, &newNPast) != 0)
        {
            throw juce::translate("Failed to decode media prompt.");
        }
        if(callback != nullptr && !callback())
        {
            return;
        }
    }
    else
    {
        auto tokens = common_tokenize(context, prompt, true, true);

        // Check if prompt tokens will exceed context capacity
        auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
        auto const ctxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
        if(ctxCapacity - ctxUsed < tokens.size())
        {
            throw juce::translate("The context capacity is not enough");
        }

        // Send the prompt tokens into the model in batches
        // Decode the prompt in chunks so prompts larger than n_batch still work.
        auto const batchSize = static_cast<size_t>(llama_n_batch(context));
        auto position = 0_z;
        while(position < tokens.size())
        {
            auto const size = std::min(batchSize, tokens.size() - position);
            auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
            auto const ret = llama_decode(context, batch);
            if(callback != nullptr && !callback())
            {
                return;
            }
            MiscWeakAssert(ret == 0);
            if(ret != 0)
            {
                throw juce::translate("Failed to decode input messages");
            }
            position += size;
        }
    }
}

static std::string retrieveMessage(llama_context* context, common_sampler* sampler, llama_vocab const* vocab, std::function<bool(std::string)> callback)
{
    // Get the assistant response with streaming support.
    std::string response;
    while(true)
    {
        // Sample the next token using common_sampler
        auto newTokenId = common_sampler_sample(sampler, context, -1);
        common_sampler_accept(sampler, newTokenId, true);
        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(vocab, newTokenId))
        {
            break;
        }

        auto const piece = common_token_to_piece(vocab, newTokenId, true);
        response += piece;
        if(callback != nullptr && !callback(response))
        {
            return response;
        }
        auto const batch = llama_batch_get_one(&newTokenId, 1);
        auto const ret = llama_decode(context, batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            throw juce::translate("Failed to decode generated messages");
        }
    }
    return response;
}

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

    auto content = parsed.value("content", nlohmann::json::array());
    auto const text = unquote(content[0].value("text", "(empty)"));
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
            static auto constexpr successMsg = "\n\nBased on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document..";
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
            bdls.push_back(
            {
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

Application::Neuralyzer::AgentLocal::~AgentLocal()
{
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates.reset();
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
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::AgentLocal", "Initialize...");

    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates.reset();
    mSamplingParams = {};
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
        mPastMessagesPosition = 0_z;
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
    params.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
    params.load_progress_callback_user_data = static_cast<void*>(this);
    params.load_progress_callback = [](float, void* data) -> bool
    {
        return !reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
    };
    params.cpuparams.n_threads = 1;
    params.cpuparams_batch.n_threads = 1;

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
    mSamplingParams = params.sampling;

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
    mSamplingParams = {};
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
        mPastMessagesPosition = 0_z;
    }
    notifyStateChanged();
    mContextMemoryUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }
    return juce::Result::ok();
}

std::vector<common_chat_msg> Application::Neuralyzer::AgentLocal::performInference()
{
    auto* model = mInitResult != nullptr ? mInitResult->model() : nullptr;
    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    auto* chatTplt = mChatTemplates.get();
    auto const* vocab = llama_model_get_vocab(model);
    MiscStrongAssert(model != nullptr && context != nullptr && chatTplt != nullptr);

    auto [messages, lastPosition] = [this]()
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        MiscWeakAssert(mPastMessagesPosition < mMessages.size());
        return std::make_pair(mMessages, mPastMessagesPosition);
    }();
    MiscWeakAssert(lastPosition < messages.size());

    common_chat_templates_inputs inputs;
    inputs.use_jinja = true;
    inputs.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
    inputs.parallel_tool_calls = true;
    inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    inputs.tools = mTools;
    inputs.add_generation_prompt = true;
    inputs.messages = std::move(messages);
    auto params = applyChatTemplates(chatTplt, inputs);
    if(lastPosition > 0_z)
    {
        // If this is not the first run, remove messages that have already been performed
        // and clear the tools
        auto const endIt = std::next(inputs.messages.begin(), static_cast<long>(lastPosition));
        inputs.messages.erase(inputs.messages.begin(), endIt);
        inputs.tools.clear();
        params.prompt = applyChatTemplates(chatTplt, inputs).prompt;
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", "Prompt: " + juce::String(params.prompt));

    // Recreate the sampler for each generation so grammar and token history start fresh.
    auto sampler = createSampler(model, mSamplingParams, params);
    if(mShouldQuit.load())
    {
        return {};
    }

    injectMessages(context, mMtmdContext.get(), inputs.messages, params.prompt, [this]()
                   {
                       updateContextMemoryUsage();
                       return !mShouldQuit.load();
                   });
    if(mShouldQuit.load())
    {
        return {};
    }

    notifyStateChanged();
    auto response = retrieveMessage(context, sampler.get(), vocab, [&, this](std::string message)
                                    {
                                        {
                                            std::unique_lock<std::mutex> lock(mTemporaryMutex);
                                            mTempResponse = message;
                                        }
                                        updateContextMemoryUsage();
                                        return !mShouldQuit.load();
                                    });
    if(mShouldQuit.load())
    {
        return {};
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", "Response: " + juce::String(response));

    common_chat_msg chatMsg;
    common_chat_parser_params parserParams(params);
    try
    {
        if(!params.parser.empty())
        {
            parserParams.parser.load(params.parser);
        }
        chatMsg = common_chat_parse(response, false, parserParams);
    }
    catch(std::exception& e)
    {
        throw juce::translate("Failed to parse model reponse: ERRSTR").replace("ERRSTR", juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        throw juce::translate("Failed to parse model reponse: ERRSTR").replace("ERRSTR", "Unknown error");
    }
    return {chatMsg};
}

juce::Result Application::Neuralyzer::AgentLocal::sendQuery(juce::String const& prompt)
{
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }

    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Summarize the conversation if there is lass than 80% of the context available
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    if(static_cast<double>(ctxUsed) / static_cast<double>(ctxCapacity) > 0.8)
    {
        auto const summarizeResult = summarizeSession();
        if(summarizeResult.failed())
        {
            return summarizeResult;
        }
    }

    common_chat_msg message;
    message.role = "user";
    message.content = prompt.toStdString();

    // Save a backup of the current session to allow recovery in case of a crash during tool calls.
    // We save it to the temp directory to avoid cluttering the user's documents with backup files,
    // and we use a fixed file name since we only need to keep the most recent backup.
    auto const tempSessionFile = getTempSessionFile("localagent.ptldoc");
    saveSession(tempSessionFile);

    // Add the new message to the history
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

        // Run the inference of the model using the full history
        std::vector<common_chat_msg> receivedMessages;
        try
        {
            receivedMessages = performInference();
        }
        catch(juce::String const& e)
        {
            notifyStateChanged();
            loadSession(tempSessionFile);
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + e);
            return juce::Result::fail(e);
        }
        catch(std::exception const& e)
        {
            notifyStateChanged();
            loadSession(tempSessionFile);
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + juce::String(e.what()));
            return juce::Result::fail(juce::String::fromUTF8(e.what()));
        }
        catch(...)
        {
            notifyStateChanged();
            loadSession(tempSessionFile);
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - Unknown error");
            return juce::Result::fail(juce::translate("Unknown error"));
        }

        if(mShouldQuit.load())
        {
            notifyStateChanged();
            loadSession(tempSessionFile);
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        if(receivedMessages.empty() || receivedMessages.at(0_z).empty())
        {
            notifyStateChanged();
            loadSession(tempSessionFile);
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - No model reponse");
            return juce::Result::fail(juce::String::fromUTF8("No model reponse"));
        }

        // Store the MCP tool call
        std::vector<common_chat_tool_call> toolCalls;
        std::string tempMessage;
        for(auto const& receivedMessage : receivedMessages)
        {
            toolCalls.insert(toolCalls.end(), receivedMessage.tool_calls.cbegin(), receivedMessage.tool_calls.cend());
        }

        // Add the received messages to the history
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.insert(mMessages.end(), receivedMessages.cbegin(), receivedMessages.cend());
            mPastMessagesPosition = mMessages.size();
        }

        saveSession(tempSessionFile);
        notifyStateChanged();

        if(toolCalls.empty())
        {
            // There are no tool calls so the last assistant message is the final answer.
            notifyStateChanged();
            return juce::Result::ok();
        }

        // Save a backup of the current session to allow recovery in case of a crash during tool calls.
        // We save it to the temp directory to avoid cluttering the user's documents with backup files,
        // and we use a fixed file name since we only need to keep the most recent backup.
        // saveSession(tempSessionFile);

        // Call the MCP tools
        for(auto const& toolCall : toolCalls)
        {
            auto toolMessage = formatToolCall(mMcpDispatcher.callTool(mMcpMethods, toolCall), false);
            // Add the tool message to the history
            {
                std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
                mMessages.push_back(std::move(toolMessage));
            }
        }
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted"));
    }

    // Max iterations reached - return the last response
    return juce::Result::fail(juce::translate("Maximum number of iterations reached"));
}

juce::Result Application::Neuralyzer::AgentLocal::addMedia(juce::File const& file)
{
    if(file == juce::File{} || !file.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The media file does not exist: " + file.getFullPathName());
        return juce::Result::fail(juce::translate("The media file does not exist: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }

    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(mMtmdContext == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "The current model does not expose a multimodal context.");
        return juce::Result::fail(juce::translate("The current model does not support media input."));
    }

    // Validate the file can be decoded now for immediate feedback. The actual tokens are fed into
    // the context lazily by performInference(), so the media survives session reloads even without
    // a cached context state (see readFiles()/getHistory() persistence of the file path below).
    auto const path = file.getFullPathName().toStdString();
    auto const wrapper = mtmd_helper_bitmap_init_from_file(mMtmdContext.get(), path.c_str(), false, mtmd_helper_init_opt_default());
    if(wrapper.bitmap == nullptr)
    {
        return juce::Result::fail(juce::translate("Failed to load media file: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }
    if(wrapper.video_ctx != nullptr)
    {
        mtmd_helper_video_free(wrapper.video_ctx);
        return juce::Result::fail(juce::translate("Video media is not supported: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }

    // Store the marker plus the source path (in tool_name, which round-trips through session
    // save/load) so performInference() can reload and tokenize the bitmap whenever it is needed.
    common_chat_msg mediaMessage;
    mediaMessage.role = "tool";
    mediaMessage.tool_name = path;
    common_chat_msg_content_part part;
    part.type = "media_marker";
    part.text = mtmd_default_marker();
    mediaMessage.content_parts.push_back(std::move(part));

    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.push_back(std::move(mediaMessage));
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
    content["type"] = "text";
    content["text"] = message;
    response["content"].push_back(content);
    return response;
}

juce::Result Application::Neuralyzer::AgentLocal::startSession()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    auto* context = mInitResult->context();
    // Clear the context memory
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);

    common_chat_msg message;
    message.role = "system";
    message.content = mMcpDispatcher.getInstructions();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages = {std::move(message)};
        mPastMessagesPosition = 0_z;
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }

    updateContextMemoryUsage();
    notifyStateChanged();
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::loadSession(juce::File const& sessionFile)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
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
    try
    {
        auto const root = nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format"));
        }

        messages = common_chat_msgs_parse_oaicompat(common_json::parse(root.at("messages").dump()));
        version = root.value("version", std::string{});
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to load session: ") + juce::String(e.what()));
    }
    // Use the latest instructions
    messages.front().content = mMcpDispatcher.getInstructions();
    auto const avoidContextState = version != mMcpDispatcher.getUuid();

    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages = std::move(messages);
        mPastMessagesPosition = 0_z;
    }

    // Clear the context memory
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);

    auto const contextFile = sessionFile.withFileExtension(".ctx");
    if(avoidContextState || contextFile.getSize() <= 0)
    {
        updateContextMemoryUsage();
        notifyStateChanged();
        return juce::Result::ok();
    }

    size_t nloaded = 0;
    std::vector<llama_token> restoredTokens(static_cast<size_t>(llama_n_ctx(context)));
    // nloaded only reflects the token count header (derived from the chat template), not the
    // actual KV-cache content, so it can be positive even though nothing was ever decoded into
    // this session (e.g. a session saved right after startSession()). Check the real memory usage
    // to detect that case and fall back to a full reprocessing of the messages.
    auto const loaded = llama_state_load_file(context, contextFile.getFullPathName().toRawUTF8(), restoredTokens.data(), restoredTokens.size(), &nloaded);
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    if(!loaded || ctxUsed <= 0)
    {
        llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);

        updateContextMemoryUsage();
        notifyStateChanged();
        return juce::Result::ok();
    }

    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mPastMessagesPosition = mMessages.size();
    }

    updateContextMemoryUsage();
    notifyStateChanged();
    MiscDebug("Application::Neuralyzer::AgentLocal", "Session loaded " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::saveSession(juce::File const& sessionFile)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file provided."));
    }
    auto const contextFile = sessionFile.withFileExtension(".ctx");

    // Ensure directory exists
    if(!sessionFile.getParentDirectory().createDirectory())
    {
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", sessionFile.getParentDirectory().getFullPathName()));
    }

    auto [messages, avoidContextState] = [this]()
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        return std::make_pair(mMessages, mMessages.size() != mPastMessagesPosition);
    }();

    try
    {
        nlohmann::json root;
        root["version"] = mMcpDispatcher.getUuid();
        for(auto const& message : messages)
        {
            root["messages"].push_back(nlohmann::json::parse(message.to_json_oaicompat().dump()));
        }

        if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
        {
            return juce::Result::fail(juce::translate("Failed to write session file"));
        }

        MiscDebug("Application::Neuralyzer::AgentRemote", "Session saved with " + juce::String(messages.size()) + " messages");
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to save session: ") + juce::String(e.what()));
    }

    if(avoidContextState)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Saved message history to files: ") + sessionFile.getFullPathName());
        return juce::Result::ok();
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    auto* context = mInitResult->context();
    common_chat_params stateParams;

    common_chat_templates_inputs inputs;
    inputs.messages = std::move(messages);
    inputs.use_jinja = true;
    inputs.add_generation_prompt = false;
    inputs.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
    inputs.parallel_tool_calls = true;
    inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    inputs.tools = mTools;
    try
    {
        stateParams = common_chat_templates_apply(mChatTemplates.get(), inputs);
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to apply chat templates: ERRSTR").replace("ERRSTR", e.what()));
    }

    auto const stateTokens = common_tokenize(context, stateParams.prompt, true, true);
    auto const saved = llama_state_save_file(context, contextFile.getFullPathName().toRawUTF8(), stateTokens.data(), stateTokens.size());
    if(!saved)
    {
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", contextFile.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Saved KV cache and message history to files: ") + contextFile.getFullPathName() + " " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentLocal::summarizeSession()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    auto const ctxAvailable = ctxCapacity - ctxUsed;
    auto const summaryPrompt = juce::String("Summarize our conversation in a concise way that preserves important details and context. This summary will replace the earlier messages in the context to free up space, so it should capture key points, decisions made, and any relevant information. The summary should be brief (MAXNUMTOKENS tokens max) but informative, allowing us to continue our discussion without losing important context.").replace("MAXNUMTOKENS", juce::String(ctxAvailable / 2));

    common_chat_msg message;
    message.role = "user";
    message.content = summaryPrompt.toStdString();

    // Add the new message to the history
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.push_back(std::move(message));
    }

    // Run the inference of the model using the full history
    std::vector<common_chat_msg> receivedMessages;
    try
    {
        receivedMessages = performInference();
    }
    catch(juce::String const& e)
    {
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + e);
        return juce::Result::fail(e);
    }
    catch(std::exception const& e)
    {
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - " + juce::String(e.what()));
        return juce::Result::fail(juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - Unknown error");
        return juce::Result::fail(juce::translate("Unknown error"));
    }

    if(mShouldQuit.load())
    {
        notifyStateChanged();
        return juce::Result::fail(juce::translate("Operation aborted"));
    }

    if(receivedMessages.empty() || receivedMessages.at(0_z).empty())
    {
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - No model reponse");
        return juce::Result::fail(juce::String::fromUTF8("No model reponse"));
    }

    auto startResult = startSession();
    if(startResult.failed())
    {
        return startResult;
    }

    // Add the new message to the history
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.insert(mMessages.end(), receivedMessages.cbegin(), receivedMessages.cend());
    }

    return juce::Result::ok();
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

class ChatParserTest
: public juce::UnitTest
{
public:
    ChatParserTest()
    : juce::UnitTest("Neuralyzer", "Application")
    {
    }

    ~ChatParserTest() override = default;

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

static ChatParserTest chatParserTest;

ANALYSE_FILE_END
