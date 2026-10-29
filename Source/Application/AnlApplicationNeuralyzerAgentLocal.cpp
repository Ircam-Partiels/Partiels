#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationInstance.h"
#include <AnlNeuralyzerData.h>
#include <AnlResourceData.h>
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

// The version of the context state saved with the sessions (the state file contains the tokens of the context memory)
static auto constexpr contextStateVersion = 1;

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_WARN)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::CharPointer_UTF8(text));
    }
}

static int32_t getNumThreads()
{
    // The CPU computes the layers that are not offloaded to the GPU (or all the layers without GPU). One core is left
    // free for the application (the threads of the inference are busy waiting).
    return std::max(common_cpu_get_num_math() - 1, 1);
}

static size_t getMemoryMargin()
{
    // The memory left free on each device when fitting the model and the context, for the application,
    // the RAG models, the multimodal projector and the system.
    static auto constexpr fixedMargin = static_cast<size_t>(4) * 1024 * 1024 * 1024;
#if JUCE_LINUX
    // The CPU backend reports the whole physical memory as free on Linux: the margin also has to keep a
    // part of the memory for the system and the other processes.
    auto const totalMemory = static_cast<size_t>(juce::SystemStats::getMemorySizeInMegabytes()) * 1024 * 1024;
    return std::max(fixedMargin, totalMemory / 4);
#else
    // The free memory reported on macOS (Metal working set) and Windows (available physical memory)
    // already excludes the memory of the system and the other processes.
    return fixedMargin;
#endif
}

static juce::String getContextFullMessage()
{
    return juce::translate("The context of the agent is full. Reset the history from the context menu of the conversation to continue.");
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

// The decoded tokens are the tokens kept in the context (the last sampled token is not decoded).
static std::string retrieveMessage(llama_context* context, llama_context* contextDraft, common_speculative* speculative, common_params_sampling const& samplingParams, common_sampler* sampler, llama_vocab const* vocab, llama_tokens& decodedTokens, std::function<bool(std::string)> callback)
{
    auto const useSpeculation = speculative != nullptr && contextDraft != nullptr;
    auto const nDraftMax = useSpeculation ? static_cast<int32_t>(common_speculative_n_max(speculative)) : 0;
    auto const contextCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    llama_tokens promptTokens;
    llama_tokens draft;
    if(useSpeculation)
    {
        common_speculative_begin(speculative, 0, promptTokens);
    }

    // Get the assistant response with streaming support.
    std::string response;
    // Sample the next token using common_sampler
    auto sampled = common_sampler_sample(sampler, context, -1);
    common_sampler_accept(sampler, sampled, true);
    while(true)
    {
        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(vocab, sampled))
        {
            break;
        }

        response += common_token_to_piece(vocab, sampled, true);
        if(callback != nullptr && !callback(response))
        {
            return response;
        }

        auto const position = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
        if(position >= contextCapacity)
        {
            throw getContextFullMessage();
        }

        // Draft the next tokens with the MTP model
        draft.clear();
        if(useSpeculation)
        {
            auto const nMax = std::min(nDraftMax, static_cast<int32_t>(contextCapacity - position - 1));
            if(nMax > 0)
            {
                // drafting, n_max, pos0, id_last, prompt, result, result_q, temp, seed
                common_speculative_get_draft_params(speculative, 0) = {true, nMax, position, sampled, &promptTokens, &draft, nullptr, samplingParams.temp, samplingParams.seed};
                common_speculative_draft(speculative);
                // The drafting step must not leave anything beyond the target position in the draft context
                llama_memory_seq_rm(llama_get_memory(contextDraft), 0, position, -1);
            }
        }

        common_batch batch(context);
        batch.add(sampled, position, 0, true);
        for(size_t i = 0_z; i < draft.size(); ++i)
        {
            batch.add(draft.at(i), position + static_cast<llama_pos>(i) + 1, 0, true);
        }
        auto const ret = llama_process(context, LLAMA_PROCESS_TYPE_DECODE, batch.get());
        MiscWeakAssert(ret == 0);
        if(ret != 0 || !common_speculative_process(speculative, batch))
        {
            throw juce::translate("Failed to decode generated messages");
        }

        if(draft.empty())
        {
            decodedTokens.push_back(sampled);
            sampled = common_sampler_sample(sampler, context, -1);
            common_sampler_accept(sampler, sampled, true);
            continue;
        }

        // Verify the draft: the accepted tokens end with the token sampled by the target model
        auto accepted = common_sampler_sample_and_accept_n(sampler, context, draft);
        MiscWeakAssert(!accepted.empty());
        auto const eogIt = std::find_if(accepted.begin(), accepted.end(), [&](auto const& token)
                                        {
                                            return llama_vocab_is_eog(vocab, token);
                                        });
        if(eogIt != accepted.end())
        {
            accepted.erase(std::next(eogIt), accepted.end());
        }

        // Discard the rejected tokens (and the end-of-generation token) from both contexts
        auto const keep = position + static_cast<llama_pos>(accepted.size());
        if(accepted.size() < draft.size() + 1_z)
        {
            if(!llama_memory_seq_rm(llama_get_memory(context), 0, keep, -1))
            {
                throw juce::translate("Failed to decode generated messages");
            }
            llama_memory_seq_rm(llama_get_memory(contextDraft), 0, keep, -1);
        }
        common_speculative_accept(speculative, 0, static_cast<uint16_t>(accepted.size() - 1_z));
        decodedTokens.push_back(sampled);
        decodedTokens.insert(decodedTokens.end(), accepted.cbegin(), std::prev(accepted.cend()));

        for(size_t i = 0_z; i + 1_z < accepted.size(); ++i)
        {
            response += common_token_to_piece(vocab, accepted.at(i), true);
            if(callback != nullptr && !callback(response))
            {
                return response;
            }
        }
        sampled = accepted.back();
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
                                         .withButton(juce::translate("Ok"));
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
    mContextCheckpoint.clear();
}

Application::Neuralyzer::AgentLocal::~AgentLocal()
{
    llama_log_set(logCallback, nullptr);
    mSpeculative.reset();
    mMtpInitResult.reset();
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

    mSpeculative.reset();
    mMtpInitResult.reset();
    mSpeculativeSynchronized.store(false);
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates.reset();
    mSamplingParams = {};
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
    }
    mContextChunks.clear();
    mContextCheckpoint.clear();
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
    params.n_ctx = 0; // Resolved by fitting the model and the context to the free device memory
    params.fit_params = true;
    params.fit_params_min_ctx = static_cast<uint32_t>(minContextSize);
    params.fit_params_target = std::vector<size_t>(llama_max_devices(), getMemoryMargin());
    // The fit requires a writable buffer of tensor buffer overrides (terminated by an empty pattern)
    while(params.tensor_buft_overrides.size() < llama_max_tensor_buft_overrides())
    {
        params.tensor_buft_overrides.push_back({nullptr, nullptr});
    }
    params.n_batch = info.batchSize.value_or(params.n_batch);
    params.sampling.min_p = info.minP.value_or(params.sampling.min_p);
    params.sampling.temp = info.temperature.value_or(params.sampling.temp);
    params.sampling.top_p = info.topP.value_or(params.sampling.top_p);
    params.sampling.top_k = info.topK.value_or(params.sampling.top_k);
    params.sampling.penalty_present = info.presencePenalty.value_or(params.sampling.penalty_present);
    params.sampling.penalty_repeat = info.repetitionPenalty.value_or(params.sampling.penalty_repeat);
    params.reasoning_format = COMMON_REASONING_FORMAT_AUTO;
    params.load_progress_callback_user_data = static_cast<void*>(this);
    params.load_progress_callback = [](float, void* data) -> bool
    {
        return !reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
    };
    params.cpuparams.n_threads = getNumThreads();
    params.cpuparams_batch.n_threads = getNumThreads();

    // Enable the multi-token prediction (MTP) speculative decoding if the MTP model is available
    auto const mtpFile = getMtpForModel(info.modelFile);
    if(mtpFile.existsAsFile())
    {
        params.speculative.types = {COMMON_SPECULATIVE_TYPE_DRAFT_MTP};
        params.speculative.draft.mparams.path = mtpFile.getFullPathName().toStdString();
        auto const limits = common_speculative_get_output_limits(params.n_batch, 1, common_speculative_n_max(&params.speculative));
        params.n_outputs_max = limits.total;
        params.n_outputs_max_per_seq = limits.per_seq;
    }

    // The fit only resolves an unset context size: the context size of the user is used as an upper limit of
    // the fitted context size so that it never exceeds the free device memory.
    auto contextSizeLimit = std::optional<int32_t>{};
    auto const initialize = [&](ggml_type cacheType)
    {
        params.cache_type_k = cacheType;
        params.cache_type_v = cacheType;
        params.speculative.draft.cache_type_k = cacheType;
        params.speculative.draft.cache_type_v = cacheType;
        params.n_ctx = 0;
        contextSizeLimit.reset();
        if(info.contextSize.has_value())
        {
            auto fitParams = params;
            auto modelParams = common_model_params_to_llama(fitParams);
            auto contextParams = common_context_params_to_llama(fitParams);
            common_fit_apply(fitParams, modelParams, contextParams);
            auto const contextSize = std::max(info.contextSize.value(), minContextSize);
            if(contextParams.n_ctx > 0)
            {
                contextSizeLimit = static_cast<int32_t>(contextParams.n_ctx);
                params.n_ctx = std::min(contextSize, contextSizeLimit.value());
            }
            else
            {
                params.n_ctx = contextSize;
            }
            MiscDebug("Application::Neuralyzer::AgentLocal", "Context size fitted to: " + juce::String(contextParams.n_ctx) + " (requested " + juce::String(contextSize) + ")");
        }
        // Initialize model, context and sampler as a single lifetime-managed object.
        mInitResult = common_init_from_params(params);
    };

    // The quantized KV cache halves the memory of the context but requires the flash attention (the quantized V
    // cache) and attention heads compatible with the block size: fall back to the default KV cache otherwise.
    // Note: the fallback reloads the whole model (the weights from the disk) although only the creation of the context
    // failed, and it occurs each time the model is initialized on a configuration that does not support the quantized
    // KV cache. It also relies on common_init_from_params() returning the model without context on failure (instead of
    // throwing), which may change with an update of llama.cpp.
    // TODO: create only the context with the default KV cache from the loaded model, or remember the failure.
    initialize(GGML_TYPE_Q8_0);
    if(!mShouldQuit.load() && mInitResult != nullptr && mInitResult->model() != nullptr && mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to create the context with the quantized KV cache: fallback to the default KV cache.");
        mInitResult.reset();
        initialize(GGML_TYPE_F16);
    }
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }
    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr)
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

    if(params.speculative.has_dft())
    {
        try
        {
            auto* targetModel = mInitResult->model();
            auto* targetContext = mInitResult->context();
            auto const seqRmType = common_context_can_seq_rm(targetContext);
            if(seqRmType == COMMON_CONTEXT_SEQ_RM_TYPE_PART || seqRmType == COMMON_CONTEXT_SEQ_RM_TYPE_RS)
            {
                auto draftParams = common_base_params_to_speculative(params);
                mMtpInitResult = common_speculative_init_from_params(draftParams, targetModel, targetContext);
                if(mMtpInitResult != nullptr && mMtpInitResult->model() != nullptr && mMtpInitResult->context() != nullptr && llama_model_n_embd_out(mMtpInitResult->model()) == llama_model_n_embd_out(targetModel))
                {
                    params.speculative.draft.ctx_tgt = targetContext;
                    params.speculative.draft.ctx_dft = mMtpInitResult->context();
                    mSpeculative.reset(common_speculative_init(params.speculative, 1));
                }
            }
        }
        catch(...)
        {
            mSpeculative.reset();
        }
        if(mSpeculative == nullptr)
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "MTP model not used: " + mtpFile.getFullPathName());
            mMtpInitResult.reset();
        }
        else
        {
            llama_set_abort_callback(mMtpInitResult->context(), [](void* data)
                                     {
                                         return reinterpret_cast<AgentLocal*>(data)->mShouldQuit.load();
                                     },
                                     static_cast<void*>(this));
            mSpeculativeSynchronized.store(true);
            MiscDebug("Application::Neuralyzer::AgentLocal", "MTP model initialized: " + mtpFile.getFullPathName());
        }
    }

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

    auto mtmdFile = getProjectorForModel(info.modelFile);
    if(mtmdFile != juce::File())
    {
        if(!mtmdFile.existsAsFile())
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "The projection file does not exist: " + mtmdFile.getFullPathName());
            return juce::Result::fail(juce::translate("The projection file does not exist: FLNAME").replace("FLNAME", mtmdFile.getFullPathName()));
        }

        auto mtmdParams = mtmd_context_params_default();
        mtmdParams.n_threads = getNumThreads();
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

    MiscDebug("Application::Neuralyzer::AgentLocal", "Successfully initialized model: " + info.modelFile.getFullPathName());
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo.modelFile = info.modelFile;
        mModelInfo.contextSize = static_cast<int32_t>(ctxCapacity);
        mModelInfo.contextSizeLimit = contextSizeLimit.value_or(static_cast<int32_t>(ctxCapacity));
        mModelInfo.batchSize = static_cast<int32_t>(batchCapacity);
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
    mSpeculative.reset();
    mMtpInitResult.reset();
    mSpeculativeSynchronized.store(false);
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates = nullptr;
    mSamplingParams = {};
    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages.clear();
    }
    mContextChunks.clear();
    mContextCheckpoint.clear();
    notifyStateChanged();
    mContextMemoryUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }
    return juce::Result::ok();
}

std::vector<common_chat_msg> Application::Neuralyzer::AgentLocal::performInference(bool useThinkingMode)
{
    auto* model = mInitResult != nullptr ? mInitResult->model() : nullptr;
    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    auto* chatTplt = mChatTemplates.get();
    auto const* vocab = llama_model_get_vocab(model);
    MiscStrongAssert(model != nullptr && context != nullptr && chatTplt != nullptr);

    common_chat_templates_inputs inputs;
    inputs.use_jinja = true;
    inputs.reasoning_format = COMMON_REASONING_FORMAT_AUTO;
    inputs.enable_thinking = useThinkingMode;
    inputs.parallel_tool_calls = true;
    inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    inputs.tools = mTools;
    inputs.add_generation_prompt = true;
    inputs.messages = getHistory();
    auto const params = applyChatTemplates(chatTplt, inputs);

    MiscDebug("Application::Neuralyzer::AgentLocal", "Prompt: " + juce::String(params.prompt.str()));

    // Recreate the sampler for each generation so grammar and token history start fresh.
    auto sampler = createSampler(model, mSamplingParams, params);
    if(mShouldQuit.load())
    {
        return {};
    }

    std::string response;
    llama_tokens decodedTokens;
    try
    {
        evaluatePrompt(inputs.messages, params, [this]()
                       {
                           updateContextMemoryUsage();
                           return !mShouldQuit.load();
                       });
        if(mShouldQuit.load())
        {
            return {};
        }

        // The speculative context must see every token of the target context to produce relevant drafts
        auto* speculative = mSpeculativeSynchronized.load() ? mSpeculative.get() : nullptr;
        auto* contextDraft = speculative != nullptr && mMtpInitResult != nullptr ? mMtpInitResult->context() : nullptr;
        notifyStateChanged();
        response = retrieveMessage(context, contextDraft, speculative, mSamplingParams, sampler.get(), vocab, decodedTokens, [&, this](std::string message)
                                   {
                                       {
                                           std::unique_lock<std::mutex> lock(mTemporaryMutex);
                                           mTempResponse = message;
                                       }
                                       updateContextMemoryUsage();
                                       return !mShouldQuit.load();
                                   });
    }
    catch(...)
    {
        // The context memory may contain tokens of a failed decoding: restore the tracked content
        appendContextTokens(decodedTokens.data(), decodedTokens.size());
        mSpeculativeSynchronized.store(false);
        rollbackContextMemory(getContextPosition());
        updateContextMemoryUsage();
        if(mShouldQuit.load())
        {
            return {};
        }
        else
        {
            throw;
        }
    }
    appendContextTokens(decodedTokens.data(), decodedTokens.size());
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
            parserParams.parser = params.parser;
        }
        chatMsg = common_chat_parse(common_chat_input(std::move(response)), false, parserParams);
    }
    catch(std::exception& e)
    {
        throw juce::translate("Failed to parse model response: ERRSTR").replace("ERRSTR", juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        throw juce::translate("Failed to parse model response: ERRSTR").replace("ERRSTR", "Unknown error");
    }
    return {chatMsg};
}

juce::Result Application::Neuralyzer::AgentLocal::sendQuery(juce::String const& prompt, bool useThinkingMode)
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

    // Summarize the conversation if there is less than 20% of the context available and if the history (without the
    // system prompt that cannot be summarized) occupies a significant part of the context, otherwise the summary would
    // not free enough space and the user has to reset the history when the context is full
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    if(static_cast<double>(ctxUsed) / static_cast<double>(ctxCapacity) > 0.8)
    {
        auto const historySize = ctxUsed - getSystemPromptSize(useThinkingMode);
        MiscDebug("Application::Neuralyzer::AgentLocal", "Context usage above 80% with a history of " + juce::String(historySize) + " positions");
        if(historySize > ctxCapacity / 4)
        {
            auto const summarizeResult = summarizeSession(useThinkingMode);
            if(summarizeResult.failed())
            {
                return summarizeResult;
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
            receivedMessages = performInference(useThinkingMode);
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

        if(receivedMessages.empty() || receivedMessages.at(0_z).empty())
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentLocal", "Inference failed - No model response");
            return juce::Result::fail(juce::translate("No model response"));
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
        }

        notifyStateChanged();

        if(toolCalls.empty())
        {
            // There are no tool calls so the last assistant message is the final answer.
            notifyStateChanged();
            return juce::Result::ok();
        }

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
        restorePoint = getHistory();
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
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    clearContextMemory();

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
    auto const avoidContextState = version != mMcpDispatcher.getUuid() || contextVersion != contextStateVersion;

    {
        std::unique_lock<std::mutex> lock(mMessagesMutex);
        mMessages = std::move(messages);
    }

    clearContextMemory();

    auto const contextFile = sessionFile.withFileExtension(".ctx");
    if(avoidContextState || contextFile.getSize() <= 0)
    {
        updateContextMemoryUsage();
        notifyStateChanged();
        return juce::Result::ok();
    }

    // The tokens saved with the state are the tokens of the context memory
    auto* context = mInitResult->context();
    size_t nloaded = 0;
    llama_tokens restoredTokens(static_cast<size_t>(llama_n_ctx(context)));
    auto const loaded = llama_state_load_file(context, contextFile.getFullPathName().toRawUTF8(), restoredTokens.data(), restoredTokens.size(), &nloaded);
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    if(!loaded || ctxUsed <= 0 || static_cast<size_t>(ctxUsed) != nloaded)
    {
        clearContextMemory();
        updateContextMemoryUsage();
        notifyStateChanged();
        return juce::Result::ok();
    }
    appendContextTokens(restoredTokens.data(), nloaded);

    // The restored state only covers the target context, so the speculative context cannot follow
    mSpeculativeSynchronized.store(false);

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

    auto const messages = getHistory();
    // The media chunks cannot be restored from the tokens saved with the state
    auto const avoidContextState = mContextChunks.size() != 1_z || !mContextChunks.front().mediaId.empty();

    try
    {
        nlohmann::json root;
        root["version"] = mMcpDispatcher.getUuid();
        root["contextVersion"] = contextStateVersion;
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

    if(avoidContextState)
    {
        contextFile.deleteFile();
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Saved message history to files: ") + sessionFile.getFullPathName());
        return juce::Result::ok();
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    auto const& stateTokens = mContextChunks.front().tokens;
    auto const saved = llama_state_save_file(mInitResult->context(), contextFile.getFullPathName().toRawUTF8(), stateTokens.data(), stateTokens.size());
    if(!saved)
    {
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", contextFile.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Saved KV cache and message history to files: ") + contextFile.getFullPathName() + " " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

llama_pos Application::Neuralyzer::AgentLocal::getSystemPromptSize(bool useThinkingMode) const
{
    // The system prompt is the common prefix of two prompts that only differ by the user message (this does not
    // depend on the template)
    auto const messages = getHistory();
    if(messages.empty() || mInitResult == nullptr || mInitResult->context() == nullptr || mChatTemplates == nullptr)
    {
        return 0;
    }
    auto const tokenize = [&](std::string const& content)
    {
        common_chat_msg message;
        message.role = "user";
        message.content = content;
        common_chat_templates_inputs inputs;
        inputs.use_jinja = true;
        inputs.reasoning_format = COMMON_REASONING_FORMAT_AUTO;
        inputs.enable_thinking = useThinkingMode;
        inputs.parallel_tool_calls = true;
        inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
        inputs.tools = mTools;
        inputs.add_generation_prompt = false;
        inputs.messages = {messages.front(), std::move(message)};
        return common_tokenize(mInitResult->context(), applyChatTemplates(mChatTemplates.get(), inputs).prompt.str(), true, true);
    };
    try
    {
        auto const first = tokenize("A");
        auto const second = tokenize("B");
        return static_cast<llama_pos>(std::distance(first.cbegin(), std::mismatch(first.cbegin(), first.cend(), second.cbegin(), second.cend()).first));
    }
    catch(...)
    {
        return 0;
    }
}

juce::Result Application::Neuralyzer::AgentLocal::summarizeSession(bool useThinkingMode)
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

    // Add the new message to the history and restore the previous history if the summary fails
    // (the context memory follows the history with the next inference)
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

    // Run the inference of the model using the full history
    std::vector<common_chat_msg> receivedMessages;
    try
    {
        receivedMessages = performInference(useThinkingMode);
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
    if(receivedMessages.empty() || receivedMessages.front().content.empty())
    {
        return restoreHistory(juce::translate("No model response"));
    }
    common_chat_msg summary;
    summary.role = "assistant";
    summary.content = receivedMessages.front().content;

    // Replace the history by the system message, the summary request and the summary (a template may require a
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

void Application::Neuralyzer::AgentLocal::resetSpeculativeMemory()
{
    if(mSpeculative == nullptr || mMtpInitResult == nullptr || mMtpInitResult->context() == nullptr)
    {
        mSpeculativeSynchronized.store(false);
        return;
    }
    llama_memory_seq_rm(llama_get_memory(mMtpInitResult->context()), 0, -1, -1);
    mSpeculativeSynchronized.store(true);
}

llama_pos Application::Neuralyzer::AgentLocal::getContextPosition() const
{
    return std::accumulate(mContextChunks.cbegin(), mContextChunks.cend(), llama_pos{0}, [](llama_pos position, ContextChunk const& chunk)
                           {
                               return position + chunk.numPositions;
                           });
}

void Application::Neuralyzer::AgentLocal::appendContextTokens(llama_token const* tokens, size_t numTokens)
{
    if(numTokens == 0_z)
    {
        return;
    }
    if(mContextChunks.empty() || !mContextChunks.back().mediaId.empty())
    {
        mContextChunks.emplace_back();
    }
    auto& chunk = mContextChunks.back();
    chunk.tokens.insert(chunk.tokens.end(), tokens, tokens + numTokens);
    chunk.numPositions = static_cast<llama_pos>(chunk.tokens.size());
}

bool Application::Neuralyzer::AgentLocal::truncateContextChunks(llama_pos position)
{
    if(!mContextCheckpoint.empty() && mContextCheckpoint.n_tokens > position)
    {
        mContextCheckpoint.clear();
    }
    auto remaining = position;
    for(auto it = mContextChunks.begin(); it != mContextChunks.end(); ++it)
    {
        if(remaining >= it->numPositions)
        {
            remaining -= it->numPositions;
            continue;
        }
        if(remaining == 0)
        {
            mContextChunks.erase(it, mContextChunks.end());
            return true;
        }
        // A media cannot be partially kept in the context
        if(!it->mediaId.empty())
        {
            return false;
        }
        it->tokens.resize(static_cast<size_t>(remaining));
        it->numPositions = remaining;
        mContextChunks.erase(std::next(it), mContextChunks.end());
        return true;
    }
    return true;
}

void Application::Neuralyzer::AgentLocal::createContextCheckpoint(llama_pos position)
{
    // The checkpoint saves the partial states (recurrent or sliding window) that cannot be rolled back
    auto* context = mInitResult->context();
    auto* memory = llama_get_memory(context);
    auto const posMin = llama_memory_seq_pos_min(memory, 0);
    if(posMin <= 0)
    {
        return;
    }
    mContextCheckpoint.clear();
    mContextCheckpoint.update_pos(position, posMin, llama_memory_seq_pos_max(memory, 0));
    mContextCheckpoint.update_tgt(context, 0, LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY);
    if(mSpeculativeSynchronized.load() && mMtpInitResult != nullptr && mMtpInitResult->context() != nullptr)
    {
        mContextCheckpoint.update_dft(mMtpInitResult->context(), 0, LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY);
        common_speculative_get_state(mSpeculative.get(), 0, mContextCheckpoint.data_spec);
    }
}

llama_pos Application::Neuralyzer::AgentLocal::rollbackContextMemory(llama_pos position)
{
    if(position <= 0)
    {
        clearContextMemory();
        return 0;
    }

    auto* context = mInitResult->context();
    auto* memory = llama_get_memory(context);
    auto* contextDraft = mSpeculativeSynchronized.load() && mMtpInitResult != nullptr ? mMtpInitResult->context() : nullptr;

    // The sliding window states cannot be rolled back before their first position (the recurrent states fail to be
    // removed partially without being modified)
    auto const numSwa = llama_model_n_swa(mInitResult->model());
    auto const posMin = llama_memory_seq_pos_min(memory, 0);
    if((numSwa <= 0 || posMin < std::max(0, position - numSwa + 2)) && llama_memory_seq_rm(memory, 0, position, -1))
    {
        if(contextDraft != nullptr && !llama_memory_seq_rm(llama_get_memory(contextDraft), 0, position, -1))
        {
            mSpeculativeSynchronized.store(false);
        }
        if(truncateContextChunks(position))
        {
            return position;
        }
        clearContextMemory();
        return 0;
    }

    // Restore the partial states of the checkpoint and remove the rest of the memory
    auto const checkpointPosition = static_cast<llama_pos>(mContextCheckpoint.n_tokens);
    if(!mContextCheckpoint.empty() && checkpointPosition <= position && mContextCheckpoint.load_tgt(context, 0, LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY) && llama_memory_seq_rm(memory, 0, checkpointPosition, -1) && truncateContextChunks(checkpointPosition))
    {
        if(contextDraft != nullptr)
        {
            if(mContextCheckpoint.load_dft(contextDraft, 0, LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY) && llama_memory_seq_rm(llama_get_memory(contextDraft), 0, checkpointPosition, -1))
            {
                common_speculative_set_state(mSpeculative.get(), 0, mContextCheckpoint.data_spec);
            }
            else
            {
                mSpeculativeSynchronized.store(false);
            }
        }
        MiscDebug("Application::Neuralyzer::AgentLocal", "Context checkpoint restored at position " + juce::String(checkpointPosition));
        return checkpointPosition;
    }

    clearContextMemory();
    return 0;
}

void Application::Neuralyzer::AgentLocal::clearContextMemory()
{
    if(mInitResult != nullptr && mInitResult->context() != nullptr)
    {
        llama_memory_seq_rm(llama_get_memory(mInitResult->context()), 0, -1, -1);
    }
    mContextChunks.clear();
    mContextCheckpoint.clear();
    resetSpeculativeMemory();
}

void Application::Neuralyzer::AgentLocal::evaluatePrompt(std::vector<common_chat_msg> const& messages, common_chat_params const& chatParams, std::function<bool(void)> callback)
{
    auto* context = mInitResult->context();
    auto* mtmd = mMtmdContext.get();
    auto const prompt = chatParams.prompt.str();

    // Create the bitmaps of all the media of the history: the prompt contains a marker for each of them
    std::vector<mtmd::bitmap_ptr> bitmaps;
    if(mtmd != nullptr)
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

    // Tokenize the full prompt into text and media chunks
    std::vector<ContextChunk> chunks;
    std::vector<mtmd_input_chunk const*> mediaChunks;
    mtmd::input_chunks inputChunks(mtmd_input_chunks_init());
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
        if(mtmd_tokenize(mtmd, inputChunks.ptr.get(), &text, bitmapPtrs.data(), bitmapPtrs.size()) != 0)
        {
            throw juce::translate("Failed to tokenize media prompt.");
        }
        for(size_t i = 0_z; i < mtmd_input_chunks_size(inputChunks.ptr.get()); ++i)
        {
            auto const* inputChunk = mtmd_input_chunks_get(inputChunks.ptr.get(), i);
            ContextChunk chunk;
            if(mtmd_input_chunk_get_type(inputChunk) == MTMD_INPUT_CHUNK_TYPE_TEXT)
            {
                auto numTokens = 0_z;
                auto const* tokens = mtmd_input_chunk_get_tokens_text(inputChunk, &numTokens);
                chunk.tokens.assign(tokens, tokens + numTokens);
                chunk.numPositions = static_cast<llama_pos>(numTokens);
                mediaChunks.push_back(nullptr);
            }
            else
            {
                auto const* id = mtmd_input_chunk_get_id(inputChunk);
                chunk.mediaId = id != nullptr && *id != '\0' ? std::string(id) : juce::Uuid().toString().toStdString();
                chunk.numPositions = mtmd_input_chunk_get_n_pos(inputChunk);
                mediaChunks.push_back(inputChunk);
            }
            chunks.push_back(std::move(chunk));
        }
    }
    else
    {
        ContextChunk chunk;
        chunk.tokens = common_tokenize(context, prompt, true, true);
        chunk.numPositions = static_cast<llama_pos>(chunk.tokens.size());
        chunks.push_back(std::move(chunk));
        mediaChunks.push_back(nullptr);
    }
    if(chunks.empty() || chunks.back().numPositions <= 0)
    {
        throw juce::translate("Failed to decode input messages");
    }

    auto const numPositions = std::accumulate(chunks.cbegin(), chunks.cend(), llama_pos{0}, [](llama_pos position, ContextChunk const& chunk)
                                              {
                                                  return position + chunk.numPositions;
                                              });
    if(numPositions > static_cast<llama_pos>(llama_n_ctx(context)))
    {
        throw getContextFullMessage();
    }

    // Find the common prefix between the context memory and the prompt
    auto numKept = llama_pos{0};
    for(size_t i = 0_z; i < std::min(chunks.size(), mContextChunks.size()); ++i)
    {
        auto const& previous = mContextChunks.at(i);
        auto const& next = chunks.at(i);
        if(!previous.mediaId.empty() || !next.mediaId.empty())
        {
            if(previous.mediaId != next.mediaId || previous.numPositions != next.numPositions)
            {
                break;
            }
            numKept += next.numPositions;
            continue;
        }
        auto const numCommon = std::distance(previous.tokens.cbegin(), std::mismatch(previous.tokens.cbegin(), previous.tokens.cend(), next.tokens.cbegin(), next.tokens.cend()).first);
        numKept += static_cast<llama_pos>(numCommon);
        if(static_cast<size_t>(numCommon) != previous.tokens.size() || static_cast<size_t>(numCommon) != next.tokens.size())
        {
            break;
        }
    }
    // At least the last token must be evaluated to sample the response
    if(numKept >= numPositions)
    {
        numKept = numPositions - (chunks.back().mediaId.empty() ? 1 : chunks.back().numPositions);
    }
    if(numKept < getContextPosition())
    {
        numKept = rollbackContextMemory(numKept);
    }
    MiscWeakAssert(numKept == getContextPosition());
    MiscDebug("Application::Neuralyzer::AgentLocal", "Context reused: " + juce::String(numKept) + "/" + juce::String(numPositions) + " positions");

    // The checkpoint is created before the generation prompt, so the next prompt can roll back the partial states
    // to the end of the history even if the template renders the last response differently
    auto checkpointPosition = numPositions - 1;
    auto const generationTokens = common_tokenize(context, chatParams.generation_prompt, false, true);
    auto const& lastTokens = chunks.back().tokens;
    if(!generationTokens.empty() && generationTokens.size() < lastTokens.size() && std::equal(generationTokens.crbegin(), generationTokens.crend(), lastTokens.crbegin()))
    {
        checkpointPosition = numPositions - static_cast<llama_pos>(generationTokens.size());
    }

    // Evaluate the rest of the prompt
    auto const batchSize = static_cast<size_t>(llama_n_batch(context));
    auto position = llama_pos{0};
    for(size_t i = 0_z; i < chunks.size(); ++i)
    {
        auto const& chunk = chunks.at(i);
        auto const chunkStart = position;
        position += chunk.numPositions;
        if(position <= numKept)
        {
            continue;
        }

        if(!chunk.mediaId.empty())
        {
            MiscWeakAssert(chunkStart == numKept);
            // The multimodal context bypasses the speculative context
            mSpeculativeSynchronized.store(false);
            llama_pos newNPast = chunkStart;
            if(mtmd_helper_eval_chunk_single(mtmd, context, mediaChunks.at(i), chunkStart, 0, static_cast<int32_t>(batchSize), i + 1_z == chunks.size(), &newNPast) != 0)
            {
                throw juce::translate("Failed to decode media prompt.");
            }
            mContextChunks.push_back(chunk);
            if(callback != nullptr && !callback())
            {
                return;
            }
            continue;
        }

        // Decode the tokens in chunks so prompts larger than n_batch still work.
        auto offset = static_cast<size_t>(std::max(numKept - chunkStart, llama_pos{0}));
        while(offset < chunk.tokens.size())
        {
            auto const tokenPosition = chunkStart + static_cast<llama_pos>(offset);
            auto size = std::min(batchSize, chunk.tokens.size() - offset);
            if(checkpointPosition > tokenPosition && checkpointPosition < tokenPosition + static_cast<llama_pos>(size))
            {
                size = static_cast<size_t>(checkpointPosition - tokenPosition);
            }
            auto* speculative = mSpeculativeSynchronized.load() ? mSpeculative.get() : nullptr;
            auto batch = common_batch_get_one(context, chunk.tokens.data() + offset, static_cast<int32_t>(size));
            auto const ret = llama_process(context, LLAMA_PROCESS_TYPE_DECODE, batch.get());
            MiscWeakAssert(ret == 0);
            if(ret != 0)
            {
                throw juce::translate("Failed to decode input messages");
            }
            if(!common_speculative_process(speculative, batch))
            {
                throw juce::translate("Failed to decode input messages");
            }
            appendContextTokens(chunk.tokens.data() + offset, size);
            offset += size;
            if(tokenPosition + static_cast<llama_pos>(size) == checkpointPosition)
            {
                createContextCheckpoint(checkpointPosition);
            }
            if(callback != nullptr && !callback())
            {
                return;
            }
        }
    }
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

            // The response of the model is parsed after the generation prompt as in AgentLocal::performInference()
            common_chat_templates_inputs inputs;
            inputs.use_jinja = true;
            inputs.parallel_tool_calls = true;
            inputs.add_generation_prompt = true;
            inputs.enable_thinking = false;
            inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
            inputs.tools = {chatTool};
            common_chat_msg msg;
            msg.role = "system";
            msg.content = "System message content";
            inputs.messages.push_back(msg);
            msg.role = "user";
            msg.content = "User message content";
            inputs.messages.push_back(msg);

            auto tplt = common_chat_templates_init(nullptr, TestResultsData::NeuralyzerChatTemplate_jinja);
            common_chat_params chatParams = common_chat_templates_apply(tplt.get(), inputs);
            common_chat_parser_params parserParams(chatParams);
            if(!chatParams.parser.empty())
            {
                parserParams.parser = chatParams.parser;
            }
            return common_chat_parse(common_chat_input(assistantMsg), false, parserParams);
        };

        // Checks that the response contains a single call of the method with the expected arguments
        auto const expectToolCall = [this](common_chat_msg const& parsed, nlohmann::json const& expected)
        {
            expectEquals(parsed.tool_calls.size(), 1_z);
            if(parsed.tool_calls.size() == 1_z)
            {
                expectEquals(juce::String(parsed.tool_calls.front().name), juce::String("my_method"));
                auto const arguments = nlohmann::json::parse(parsed.tool_calls.front().arguments);
                expect(arguments == expected, arguments.dump() + " != " + expected.dump());
            }
        };

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

        // The grammar of the tool calls (applied by the sampler during the generation) requires the required parameter
        // first, then the optional parameters in any order, and lower case booleans: the parser rejects the responses
        // that the grammar prevents. The string values are not quoted.
        beginTest("Chat Parser Tools");
        {
            testCase("1 - Required Parameter Only", [&]()
                     {
                         expectToolCall(parseToolCall(
                                            "<tool_call>\n"
                                            "<function=my_method>\n"
                                            "<parameter=identifier>\n"
                                            "zaza\n"
                                            "</parameter>\n"
                                            "</function>\n"
                                            "</tool_call>"),
                                        {{"identifier", "zaza"}});
                     });
            testCase("2 - Optional Parameter Before Required Parameter", [&]()
                     {
                         expect(parseToolCall(
                                    "<tool_call>\n"
                                    "<function=my_method>\n"
                                    "<parameter=enabled>\n"
                                    "true\n"
                                    "</parameter>\n"
                                    "<parameter=identifier>\n"
                                    "zaza\n"
                                    "</parameter>\n"
                                    "</function>\n"
                                    "</tool_call>")
                                    .tool_calls.empty());
                     });
            testCase("3 - Ordered Parameters", [&]()
                     {
                         expectToolCall(parseToolCall(
                                            "<tool_call>\n"
                                            "<function=my_method>\n"
                                            "<parameter=identifier>\n"
                                            "zaza\n"
                                            "</parameter>\n"
                                            "<parameter=enabled>\n"
                                            "true\n"
                                            "</parameter>\n"
                                            "</function>\n"
                                            "</tool_call>"),
                                        {{"identifier", "zaza"}, {"enabled", true}});
                     });
            testCase("4 - Optional Parameters Before Required Parameter", [&]()
                     {
                         expect(parseToolCall(
                                    "<tool_call>\n"
                                    "<function=my_method>\n"
                                    "<parameter=count>\n"
                                    "12\n"
                                    "</parameter>\n"
                                    "<parameter=enabled>\n"
                                    "true\n"
                                    "</parameter>\n"
                                    "<parameter=identifier>\n"
                                    "zaza\n"
                                    "</parameter>\n"
                                    "</function>\n"
                                    "</tool_call>")
                                    .tool_calls.empty());
                     });
            testCase("5 - Reordered Optional Parameters", [&]()
                     {
                         expectToolCall(parseToolCall(
                                            "<tool_call>\n"
                                            "<function=my_method>\n"
                                            "<parameter=identifier>\n"
                                            "zaza\n"
                                            "</parameter>\n"
                                            "<parameter=count>\n"
                                            "12\n"
                                            "</parameter>\n"
                                            "<parameter=enabled>\n"
                                            "true\n"
                                            "</parameter>\n"
                                            "</function>\n"
                                            "</tool_call>"),
                                        {{"identifier", "zaza"}, {"count", 12}, {"enabled", true}});
                     });
            testCase("6 - Upper case Boolean", [&]()
                     {
                         expect(parseToolCall(
                                    "<tool_call>\n"
                                    "<function=my_method>\n"
                                    "<parameter=identifier>\n"
                                    "zaza\n"
                                    "</parameter>\n"
                                    "<parameter=enabled>\n"
                                    "True\n"
                                    "</parameter>\n"
                                    "</function>\n"
                                    "</tool_call>")
                                    .tool_calls.empty());
                     });
        }
    }
};

static ChatParserTest chatParserTest;

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
        ask("My name is Zorglub and my favorite number is 47.", {});
        ask("What is my name?", "Zorglub");
        ask("What is my favorite number plus 3?", "50");

        beginTest("Session");
        auto const sessionFile = juce::File::createTempFile("ptldoc");
        expect(agent.saveSession(sessionFile).wasOk());
        expect(agent.loadSession(sessionFile).wasOk());
        ask("What was my favorite number?", "47");
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
            ask("What is my name?", "Zorglub");
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

        // The model may refuse to answer about personal information without reasoning, the questions use the name
        // of the document
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
        expect(result.failed() && result.getErrorMessage() == getContextFullMessage(), result.getErrorMessage());
        expect(agent.getHistory().size() == 5_z, juce::String(agent.getHistory().size()));
    }
};

static AgentLocalTest agentLocalTest;

ANALYSE_FILE_END
