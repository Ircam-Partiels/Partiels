#include "AnlApplicationNeuralyzerEngineLocal.h"
#include <TestResultsData.h>

ANALYSE_FILE_BEGIN

// The version of the state files (the state file contains the tokens of the context memory)
static auto constexpr stateVersion = 1;

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_WARN)
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", juce::CharPointer_UTF8(text));
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

static common_chat_templates_inputs createTemplateInputs(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode, bool addGenerationPrompt)
{
    common_chat_templates_inputs inputs;
    inputs.use_jinja = true;
    inputs.reasoning_format = COMMON_REASONING_FORMAT_AUTO;
    inputs.enable_thinking = useThinkingMode;
    inputs.parallel_tool_calls = true;
    inputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_AUTO;
    inputs.tools = tools;
    inputs.add_generation_prompt = addGenerationPrompt;
    inputs.messages = messages;
    return inputs;
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

// The decoded tokens are the tokens kept in the context (the last sampled token is not decoded). The tokens are decoded
// from the start position (the positions of the context memory do not follow the positions of the multimodal chunks)
// and the context memory already contains the number of used tokens.
static std::string retrieveMessage(llama_context* context, llama_context* contextDraft, common_speculative* speculative, common_params_sampling const& samplingParams, common_sampler* sampler, llama_vocab const* vocab, llama_pos startPosition, size_t numUsedTokens, llama_tokens& decodedTokens, std::function<bool(std::string)> callback)
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

    std::string response;
    auto sampled = common_sampler_sample(sampler, context, -1);
    common_sampler_accept(sampler, sampled, true);
    while(true)
    {
        if(llama_vocab_is_eog(vocab, sampled))
        {
            break;
        }

        response += common_token_to_piece(vocab, sampled, true);
        if(callback != nullptr && !callback(response))
        {
            return response;
        }

        auto const position = startPosition + static_cast<llama_pos>(decodedTokens.size());
        auto const numAvailableTokens = contextCapacity - static_cast<llama_pos>(numUsedTokens + decodedTokens.size());
        if(numAvailableTokens <= 0)
        {
            throw Application::Neuralyzer::EngineLocal::getContextFullMessage();
        }

        // Draft the next tokens with the MTP model
        draft.clear();
        if(useSpeculation)
        {
            auto const nMax = std::min(nDraftMax, static_cast<int32_t>(numAvailableTokens - 1));
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

Application::Neuralyzer::EngineLocal::EngineLocal(std::atomic<bool> const& shouldAbort)
: mShouldAbort(shouldAbort)
{
    mContextCheckpoint.clear();
}

Application::Neuralyzer::EngineLocal::~EngineLocal()
{
    llama_log_set(logCallback, nullptr);
    reset();
}

juce::Result Application::Neuralyzer::EngineLocal::initialize(ModelInfo const& info, juce::File const& projectorFile, juce::File const& mtpFile)
{
    llama_log_set(logCallback, nullptr);
    reset();

    if(info.modelFile == juce::File())
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "The model file is not set.");
        return juce::Result::fail(juce::translate("The model file is not set."));
    }
    if(!info.modelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "The model file does not exist: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("The model file does not exist: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
    }

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
        return !reinterpret_cast<EngineLocal*>(data)->mShouldAbort.load();
    };
    params.cpuparams.n_threads = getNumThreads();
    params.cpuparams_batch.n_threads = getNumThreads();

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
    auto const initializeModel = [&](ggml_type cacheType)
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
            MiscDebug("Application::Neuralyzer::EngineLocal", "Context size fitted to: " + juce::String(contextParams.n_ctx) + " (requested " + juce::String(contextSize) + ")");
        }
        mInitResult = common_init_from_params(params);
    };

    // The quantized KV cache halves the memory of the context but requires the flash attention (the quantized V
    // cache) and attention heads compatible with the block size: fall back to the default KV cache otherwise.
    // Note: the fallback reloads the whole model (the weights from the disk) although only the creation of the context
    // failed, and it occurs each time the model is initialized on a configuration that does not support the quantized
    // KV cache. It also relies on common_init_from_params() returning the model without context on failure (instead of
    // throwing), which may change with an update of llama.cpp.
    // TODO: create only the context with the default KV cache from the loaded model, or remember the failure.
    initializeModel(GGML_TYPE_Q8_0);
    if(!mShouldAbort.load() && mInitResult != nullptr && mInitResult->model() != nullptr && mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "Failed to create the context with the quantized KV cache: fallback to the default KV cache.");
        mInitResult.reset();
        initializeModel(GGML_TYPE_F16);
    }
    if(mShouldAbort.load())
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "Model loading aborted by user.");
        reset();
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }
    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "Failed to load model from: " + info.modelFile.getFullPathName());
        reset();
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
    }
    mSamplingParams = params.sampling;

    llama_set_abort_callback(mInitResult->context(), [](void* data)
                             {
                                 return reinterpret_cast<EngineLocal*>(data)->mShouldAbort.load();
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
            MiscDebug("Application::Neuralyzer::EngineLocal", "MTP model not used: " + mtpFile.getFullPathName());
            mMtpInitResult.reset();
        }
        else
        {
            llama_set_abort_callback(mMtpInitResult->context(), [](void* data)
                                     {
                                         return reinterpret_cast<EngineLocal*>(data)->mShouldAbort.load();
                                     },
                                     static_cast<void*>(this));
            mSpeculativeSynchronized.store(true);
            MiscDebug("Application::Neuralyzer::EngineLocal", "MTP model initialized: " + mtpFile.getFullPathName());
        }
    }

    auto const* context = mInitResult->context();
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    auto const batchCapacity = static_cast<size_t>(llama_n_batch(context));
    MiscDebug("Application::Neuralyzer::EngineLocal", "Model, context, and sampler loaded successfully with context size: " + juce::String(ctxCapacity) + " and batch size: " + juce::String(batchCapacity));

    if(!params.chat_template.empty() && !common_chat_verify_template(params.chat_template, true))
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "The chat template is not supported: " + templateFile.getFullPathName());
        reset();
        return juce::Result::fail(juce::translate("The chat template is not supported: FLNAME").replace("FLNAME", templateFile.getFullPathName()));
    }
    try
    {
        mChatTemplates = common_chat_templates_init(mInitResult->model(), params.chat_template);
    }
    catch(...)
    {
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::EngineLocal", "Failed to initialize chat templates: fallback to default.");
        mChatTemplates = common_chat_templates_init(mInitResult->model(), std::string{});
    }
    if(mChatTemplates == nullptr)
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "Failed to initialize chat templates.");
        reset();
        return juce::Result::fail(juce::translate("Failed to initialize chat templates"));
    }

    if(projectorFile != juce::File())
    {
        if(!projectorFile.existsAsFile())
        {
            MiscDebug("Application::Neuralyzer::EngineLocal", "The projection file does not exist: " + projectorFile.getFullPathName());
            reset();
            return juce::Result::fail(juce::translate("The projection file does not exist: FLNAME").replace("FLNAME", projectorFile.getFullPathName()));
        }

        auto mtmdParams = mtmd_context_params_default();
        mtmdParams.n_threads = getNumThreads();
        mtmdParams.progress_callback_user_data = static_cast<void*>(this);
        mtmdParams.progress_callback = [](float, void* data) -> bool
        {
            return !reinterpret_cast<EngineLocal*>(data)->mShouldAbort.load();
        };

        try
        {
            mMtmdContext.reset(mtmd_init_from_file(projectorFile.getFullPathName().toRawUTF8(), mInitResult->model(), mtmdParams));
        }
        catch(...)
        {
            MiscWeakAssert(false);
            mMtmdContext.reset();
        }
        if(mMtmdContext == nullptr)
        {
            reset();
            return juce::Result::fail(juce::translate("Failed to initialize multimodal context from projection file: FLNAME").replace("FLNAME", projectorFile.getFullPathName()));
        }
        MiscDebug("Application::Neuralyzer::EngineLocal", "Multimodal context initialized successfully from projection file: " + projectorFile.getFullPathName());
    }

    if(mShouldAbort.load())
    {
        MiscDebug("Application::Neuralyzer::EngineLocal", "Model loading aborted by user.");
        reset();
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    MiscDebug("Application::Neuralyzer::EngineLocal", "Successfully initialized model: " + info.modelFile.getFullPathName());
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

void Application::Neuralyzer::EngineLocal::reset()
{
    mSpeculative.reset();
    mMtpInitResult.reset();
    mSpeculativeSynchronized.store(false);
    mInitResult.reset();
    mMtmdContext.reset();
    mChatTemplates.reset();
    mSamplingParams = {};
    mContextChunks.clear();
    mContextCheckpoint.clear();
    mContextUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }
}

bool Application::Neuralyzer::EngineLocal::isInitialized() const
{
    return mInitResult != nullptr && mInitResult->model() != nullptr && mInitResult->context() != nullptr && mChatTemplates != nullptr;
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::EngineLocal::getModelInfo() const
{
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    return mModelInfo;
}

float Application::Neuralyzer::EngineLocal::getContextUsage() const
{
    return mContextUsage.load();
}

common_chat_msg Application::Neuralyzer::EngineLocal::generate(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode, std::function<void()> onGenerationStarted, std::function<void(std::string const&)> onResponseChanged)
{
    MiscStrongAssert(isInitialized());
    llama_log_set(logCallback, nullptr);
    auto* model = mInitResult->model();
    auto* context = mInitResult->context();
    auto const* vocab = llama_model_get_vocab(model);

    auto inputs = createTemplateInputs(messages, tools, useThinkingMode, true);
    // The media files are reloaded with each generation: a media file that has been moved or deleted is replaced by a
    // text so the conversation can continue (the messages keep the media in case the file is restored)
    for(auto& message : inputs.messages)
    {
        if(message.contains_media() && !juce::File(juce::String::fromUTF8(message.tool_name.c_str())).existsAsFile())
        {
            MiscDebug("Application::Neuralyzer::EngineLocal", "The media file is not available: " + juce::String::fromUTF8(message.tool_name.c_str()));
            message.content_parts.clear();
            message.content = "The media file '" + message.tool_name + "' is no longer available.";
        }
    }
    auto const params = applyChatTemplates(mChatTemplates.get(), inputs);
    MiscDebug("Application::Neuralyzer::EngineLocal", "Prompt: " + juce::String(params.prompt.str()));

    // The sampler is recreated for each generation so the grammar and the token history start fresh
    auto sampler = createSampler(model, mSamplingParams, params);
    if(mShouldAbort.load())
    {
        return {};
    }

    std::string response;
    llama_tokens decodedTokens;
    try
    {
        evaluatePrompt(inputs.messages, params);
        if(mShouldAbort.load())
        {
            return {};
        }

        // The speculative context must see every token of the target context to produce relevant drafts
        auto* speculative = mSpeculativeSynchronized.load() ? mSpeculative.get() : nullptr;
        auto* contextDraft = speculative != nullptr && mMtpInitResult != nullptr ? mMtpInitResult->context() : nullptr;
        if(onGenerationStarted != nullptr)
        {
            onGenerationStarted();
        }
        response = retrieveMessage(context, contextDraft, speculative, mSamplingParams, sampler.get(), vocab, getContextPosition(), getContextSize(), decodedTokens, [&, this](std::string message)
                                   {
                                       if(onResponseChanged != nullptr)
                                       {
                                           onResponseChanged(message);
                                       }
                                       updateContextUsage();
                                       return !mShouldAbort.load();
                                   });
    }
    catch(...)
    {
        // The context memory may contain tokens of a failed decoding: restore the tracked content
        appendContextTokens(decodedTokens.data(), decodedTokens.size());
        mSpeculativeSynchronized.store(false);
        rollbackContextMemory(getContextPosition());
        updateContextUsage();
        if(mShouldAbort.load())
        {
            return {};
        }
        throw;
    }
    appendContextTokens(decodedTokens.data(), decodedTokens.size());
    if(mShouldAbort.load())
    {
        return {};
    }

    MiscDebug("Application::Neuralyzer::EngineLocal", "Response: " + juce::String(response));
    common_chat_parser_params parserParams(params);
    try
    {
        if(!params.parser.empty())
        {
            parserParams.parser = params.parser;
        }
        return common_chat_parse(common_chat_input(std::move(response)), false, parserParams);
    }
    catch(std::exception& e)
    {
        throw juce::translate("Failed to parse model response: ERRSTR").replace("ERRSTR", juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        throw juce::translate("Failed to parse model response: ERRSTR").replace("ERRSTR", "Unknown error");
    }
}

void Application::Neuralyzer::EngineLocal::clearContext()
{
    clearContextMemory();
    updateContextUsage();
}

size_t Application::Neuralyzer::EngineLocal::getContextSize() const
{
    return std::accumulate(mContextChunks.cbegin(), mContextChunks.cend(), 0_z, [](size_t size, ContextChunk const& chunk)
                           {
                               return size + chunk.numTokens;
                           });
}

size_t Application::Neuralyzer::EngineLocal::getContextCapacity() const
{
    return mInitResult != nullptr && mInitResult->context() != nullptr ? static_cast<size_t>(llama_n_ctx(mInitResult->context())) : 0_z;
}

size_t Application::Neuralyzer::EngineLocal::getSystemPromptSize(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode) const
{
    if(messages.empty() || !isInitialized())
    {
        return 0_z;
    }
    // The system prompt is the common prefix of two prompts that only differ by the user message (this does not
    // depend on the template)
    auto const tokenize = [&](std::string const& content)
    {
        common_chat_msg message;
        message.role = "user";
        message.content = content;
        auto const inputs = createTemplateInputs({messages.front(), std::move(message)}, tools, useThinkingMode, false);
        return common_tokenize(mInitResult->context(), applyChatTemplates(mChatTemplates.get(), inputs).prompt.str(), true, true);
    };
    try
    {
        auto const first = tokenize("A");
        auto const second = tokenize("B");
        return static_cast<size_t>(std::distance(first.cbegin(), std::mismatch(first.cbegin(), first.cend(), second.cbegin(), second.cend()).first));
    }
    catch(...)
    {
        return 0_z;
    }
}

bool Application::Neuralyzer::EngineLocal::supportsMedia() const
{
    return mMtmdContext != nullptr;
}

juce::Result Application::Neuralyzer::EngineLocal::checkMedia(juce::File const& file) const
{
    MiscWeakAssert(supportsMedia());
    if(!supportsMedia())
    {
        return juce::Result::fail(juce::translate("The current model does not support media input."));
    }
    auto const wrapper = mtmd_helper_bitmap_init_from_file(mMtmdContext.get(), file.getFullPathName().toRawUTF8(), false, mtmd_helper_init_opt_default());
    if(wrapper.bitmap == nullptr)
    {
        return juce::Result::fail(juce::translate("Failed to load media file: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }
    mtmd::bitmap_ptr const bitmap(wrapper.bitmap);
    if(wrapper.video_ctx != nullptr)
    {
        mtmd_helper_video_free(wrapper.video_ctx);
        return juce::Result::fail(juce::translate("Video media is not supported: FLNAME.").replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

common_chat_msg Application::Neuralyzer::EngineLocal::createMediaMessage(juce::File const& file)
{
    // The path is stored in the tool name that is saved with the sessions
    common_chat_msg message;
    message.role = "tool";
    message.tool_name = file.getFullPathName().toStdString();
    common_chat_msg_content_part part;
    part.type = "media_marker";
    part.text = mtmd_default_marker();
    message.content_parts.push_back(std::move(part));
    return message;
}

int Application::Neuralyzer::EngineLocal::getStateVersion()
{
    return stateVersion;
}

juce::Result Application::Neuralyzer::EngineLocal::saveState(juce::File const& file) const
{
    if(!isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    // The media chunks cannot be restored from the tokens saved with the state
    if(mContextChunks.size() != 1_z || !mContextChunks.front().mediaId.empty())
    {
        file.deleteFile();
        return juce::Result::ok();
    }

    llama_log_set(logCallback, nullptr);
    auto const& tokens = mContextChunks.front().tokens;
    if(!llama_state_save_file(mInitResult->context(), file.getFullPathName().toRawUTF8(), tokens.data(), tokens.size()))
    {
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", file.getFullPathName()));
    }
    MiscDebug("Application::Neuralyzer::EngineLocal", "Saved KV cache to file: " + file.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::EngineLocal::loadState(juce::File const& file)
{
    if(!isInitialized())
    {
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    clearContextMemory();

    auto* context = mInitResult->context();
    size_t numLoadedTokens = 0;
    llama_tokens tokens(static_cast<size_t>(llama_n_ctx(context)));
    auto const loaded = llama_state_load_file(context, file.getFullPathName().toRawUTF8(), tokens.data(), tokens.size(), &numLoadedTokens);
    auto const numUsedPositions = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    if(!loaded || numUsedPositions <= 0 || static_cast<size_t>(numUsedPositions) != numLoadedTokens)
    {
        clearContextMemory();
        updateContextUsage();
        return juce::Result::fail(juce::translate("Failed to load state from file: FLNAME").replace("FLNAME", file.getFullPathName()));
    }
    appendContextTokens(tokens.data(), numLoadedTokens);
    // The restored state only covers the target context, so the speculative context cannot follow
    mSpeculativeSynchronized.store(false);
    updateContextUsage();
    return juce::Result::ok();
}

juce::String Application::Neuralyzer::EngineLocal::getContextFullMessage()
{
    return juce::translate("The context of the agent is full. Reset the history from the context menu of the conversation to continue.");
}

void Application::Neuralyzer::EngineLocal::evaluatePrompt(std::vector<common_chat_msg> const& messages, common_chat_params const& chatParams)
{
    auto* context = mInitResult->context();
    auto* mtmd = mMtmdContext.get();
    auto const prompt = chatParams.prompt.str();

    // Create the bitmaps of all the media of the messages: the prompt contains a marker for each of them
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
                chunk.numTokens = numTokens;
                mediaChunks.push_back(nullptr);
            }
            else
            {
                auto const* id = mtmd_input_chunk_get_id(inputChunk);
                chunk.mediaId = id != nullptr && *id != '\0' ? std::string(id) : juce::Uuid().toString().toStdString();
                chunk.numPositions = mtmd_input_chunk_get_n_pos(inputChunk);
                chunk.numTokens = mtmd_input_chunk_get_n_tokens(inputChunk);
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
        chunk.numTokens = chunk.tokens.size();
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
    // The capacity of the context memory is a number of tokens (a media uses more tokens than positions)
    auto const numTokens = std::accumulate(chunks.cbegin(), chunks.cend(), 0_z, [](size_t size, ContextChunk const& chunk)
                                           {
                                               return size + chunk.numTokens;
                                           });
    if(numTokens > static_cast<size_t>(llama_n_ctx(context)))
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
    MiscDebug("Application::Neuralyzer::EngineLocal", "Context reused: " + juce::String(numKept) + "/" + juce::String(numPositions) + " positions");

    // The checkpoint is created before the generation prompt, so the next prompt can roll back the partial states
    // to the end of the history even if the template renders the last response differently
    auto checkpointPosition = numPositions - 1;
    auto const generationTokens = common_tokenize(context, chatParams.generation_prompt, false, true);
    auto const& lastTokens = chunks.back().tokens;
    if(!generationTokens.empty() && generationTokens.size() < lastTokens.size() && std::equal(generationTokens.crbegin(), generationTokens.crend(), lastTokens.crbegin()))
    {
        checkpointPosition = numPositions - static_cast<llama_pos>(generationTokens.size());
    }

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
            updateContextUsage();
            if(mShouldAbort.load())
            {
                return;
            }
            continue;
        }

        // The tokens are decoded in batches so prompts larger than n_batch still work
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
            // The positions are explicit because the positions of the context memory do not follow the multimodal chunks
            common_batch batch(context);
            for(size_t k = 0_z; k < size; ++k)
            {
                batch.add(chunk.tokens.at(offset + k), tokenPosition + static_cast<llama_pos>(k), 0, k + 1_z == size);
            }
            auto const ret = llama_process(context, LLAMA_PROCESS_TYPE_DECODE, batch.get());
            MiscWeakAssert(ret == 0);
            if(ret != 0 || !common_speculative_process(speculative, batch))
            {
                throw juce::translate("Failed to decode input messages");
            }
            appendContextTokens(chunk.tokens.data() + offset, size);
            offset += size;
            if(tokenPosition + static_cast<llama_pos>(size) == checkpointPosition)
            {
                createContextCheckpoint(checkpointPosition);
            }
            updateContextUsage();
            if(mShouldAbort.load())
            {
                return;
            }
        }
    }
}

llama_pos Application::Neuralyzer::EngineLocal::getContextPosition() const
{
    return std::accumulate(mContextChunks.cbegin(), mContextChunks.cend(), llama_pos{0}, [](llama_pos position, ContextChunk const& chunk)
                           {
                               return position + chunk.numPositions;
                           });
}

void Application::Neuralyzer::EngineLocal::appendContextTokens(llama_token const* tokens, size_t numTokens)
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
    chunk.numTokens = chunk.tokens.size();
}

bool Application::Neuralyzer::EngineLocal::truncateContextChunks(llama_pos position)
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
        if(!it->mediaId.empty())
        {
            return false;
        }
        it->tokens.resize(static_cast<size_t>(remaining));
        it->numPositions = remaining;
        it->numTokens = it->tokens.size();
        mContextChunks.erase(std::next(it), mContextChunks.end());
        return true;
    }
    return true;
}

void Application::Neuralyzer::EngineLocal::createContextCheckpoint(llama_pos position)
{
    auto* context = mInitResult->context();
    auto* memory = llama_get_memory(context);
    auto const posMin = llama_memory_seq_pos_min(memory, 0);
    // The memory without partial states can be rolled back without checkpoint
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

llama_pos Application::Neuralyzer::EngineLocal::rollbackContextMemory(llama_pos position)
{
    if(position <= 0)
    {
        clearContextMemory();
        return 0;
    }

    auto* context = mInitResult->context();
    auto* memory = llama_get_memory(context);
    auto* contextDraft = mSpeculativeSynchronized.load() && mMtpInitResult != nullptr ? mMtpInitResult->context() : nullptr;

    if(position >= llama_memory_seq_pos_max(memory, 0) + 1)
    {
        if(truncateContextChunks(position))
        {
            return std::min(position, getContextPosition());
        }
        clearContextMemory();
        return 0;
    }

    // The memory can be removed partially if it only contains the states of each position (attention), or if the
    // sliding window states still cover the position. The recurrent states only keep the state of the last position
    // (llama.cpp accepts the partial removal of the last positions for the speculative decoding but the state is only
    // valid within the last decoded batch), so they are restored from the checkpoint.
    auto const numSwa = llama_model_n_swa(mInitResult->model());
    auto const posMin = llama_memory_seq_pos_min(memory, 0);
    auto const canRemove = numSwa > 0 ? posMin < std::max(0, position - numSwa + 2) : posMin == 0;
    if(canRemove && llama_memory_seq_rm(memory, 0, position, -1))
    {
        if(contextDraft != nullptr && !llama_memory_seq_rm(llama_get_memory(contextDraft), 0, position, -1))
        {
            mSpeculativeSynchronized.store(false);
        }
        MiscDebug("Application::Neuralyzer::EngineLocal", "Context memory rolled back from position " + juce::String(getContextPosition()) + " to " + juce::String(position));
        if(truncateContextChunks(position))
        {
            return position;
        }
        clearContextMemory();
        return 0;
    }

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
        MiscDebug("Application::Neuralyzer::EngineLocal", "Context checkpoint restored at position " + juce::String(checkpointPosition));
        return checkpointPosition;
    }

    MiscDebug("Application::Neuralyzer::EngineLocal", "Context memory cleared to roll back from position " + juce::String(getContextPosition()) + " to " + juce::String(position));
    clearContextMemory();
    return 0;
}

void Application::Neuralyzer::EngineLocal::clearContextMemory()
{
    if(mInitResult != nullptr && mInitResult->context() != nullptr)
    {
        llama_memory_seq_rm(llama_get_memory(mInitResult->context()), 0, -1, -1);
    }
    mContextChunks.clear();
    mContextCheckpoint.clear();
    resetSpeculativeMemory();
}

void Application::Neuralyzer::EngineLocal::resetSpeculativeMemory()
{
    if(mSpeculative == nullptr || mMtpInitResult == nullptr || mMtpInitResult->context() == nullptr)
    {
        mSpeculativeSynchronized.store(false);
        return;
    }
    llama_memory_seq_rm(llama_get_memory(mMtpInitResult->context()), 0, -1, -1);
    mSpeculativeSynchronized.store(true);
}

void Application::Neuralyzer::EngineLocal::updateContextUsage()
{
    auto const capacity = getContextCapacity();
    mContextUsage.store(capacity > 0_z ? static_cast<float>(static_cast<double>(getContextSize()) / static_cast<double>(capacity)) : 0.0f);
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

            // The response of the model is parsed after the generation prompt as in EngineLocal::generate()
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

ANALYSE_FILE_END
