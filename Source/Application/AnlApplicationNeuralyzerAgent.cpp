#include "AnlApplicationNeuralyzerAgent.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

ANALYSE_FILE_BEGIN

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_ERROR)
    {
        MiscDebug("Application::Neuralyzer::Agent", text);
    }
}

static std::pair<int, std::string> parseToolCalls(Application::Neuralyzer::Mcp::Dispatcher& dispatcher, common_chat_params const& chatParams, std::string const& assistantResponse)
{
    auto const toolCalls = [&]()
    {
        auto const createResults = [](juce::Result result, std::vector<common_chat_tool_call> tc = {})
        {
            return std::make_tuple(std::move(result), std::move(tc));
        };

        if(assistantResponse.empty())
        {
            return createResults(juce::Result::ok());
        }
        try
        {
            // Use the appropriate parser based on format type
            auto tc = [&]()
            {
                // Create parser config from stored chat params
                common_chat_parser_params parserParams(chatParams);
                if(!chatParams.parser.empty())
                {
                    parserParams.parser.load(chatParams.parser);
                }
                // Use generic parser for all other formats
                return common_chat_parse(assistantResponse, false, parserParams).tool_calls;
            }();

            // Tool calls are now in parsed.tool_calls
            if(tc.empty())
            {
                MiscDebug("Application::Neuralyzer::Agent", "No tool calls found in response");
                return createResults(juce::Result::ok());
            }

            MiscDebug("Application::Neuralyzer::Agent", juce::String("Parsed ") + juce::String(tc.size()) + juce::String(" tool calls"));
            return createResults(juce::Result::ok(), std::move(tc));
        }
        catch(std::exception const& e)
        {
            MiscDebug("Application::Neuralyzer::Agent", e.what());
            return createResults(juce::Result::fail(juce::String("Error parsing tool calls: ") + e.what() + ". Ensure the tool calls are properly formatted according to the expected structure, the letter case is respected (lower case for JSON boolean), the JSON and/or XML formats are well-formatted."));
        }
        catch(...)
        {
            return createResults(juce::Result::fail("Error parsing tool calls: Unknown error"));
        }
    }();

    if(std::get<0>(toolCalls).failed())
    {
        return std::make_pair(2, std::get<0>(toolCalls).getErrorMessage().toStdString() + "\nRespond again, correcting the error.");
    }
    if(std::get<1>(toolCalls).empty())
    {
        return std::make_pair(0, assistantResponse);
    }
    std::string results = "Tool execution results:\n\n";
    bool allSuccess = true;
    for(auto const& toolCall : std::get<1>(toolCalls))
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Calling tool: ") + toolCall.name + " (ID: " + toolCall.id + ")");

        results += "[Tool Call ID: " + toolCall.id + "]\n";
        results += "Tool: " + toolCall.name + "\n";

        nlohmann::json request;
        request["method"] = "tools/call";
        request["params"]["name"] = toolCall.name;
        request["params"]["arguments"] = toolCall.arguments.empty() ? nlohmann::json::object() : nlohmann::json::parse(toolCall.arguments);

        try
        {
            auto const dispatcherResponse = dispatcher.handle(request);
            if(dispatcherResponse.contains("error"))
            {
                allSuccess = false;
                results += "Status: ERROR\n";
                if(dispatcherResponse.at("error").is_object())
                {
                    auto const& error = dispatcherResponse.at("error");
                    if(error.contains("message") && error.at("message").is_string())
                    {
                        results += "Message: " + error.at("message").get<std::string>() + "\n";
                    }
                    if(error.contains("code") && error.at("code").is_number())
                    {
                        results += "Code: " + std::to_string(error.at("code").get<int>()) + "\n";
                    }
                }
                else
                {
                    results += "Message: " + dispatcherResponse.at("error").dump() + "\n";
                }
            }
            else
            {
                auto const isError = dispatcherResponse.contains("isError") && dispatcherResponse.at("isError").get<bool>();
                if(isError)
                {
                    allSuccess = false;
                    results += "Status: FAILED\n";
                    if(dispatcherResponse.contains("content") && dispatcherResponse.at("content").is_array() && !dispatcherResponse.at("content").empty() && dispatcherResponse.at("content").at(0).contains("text"))
                    {
                        results += "Message: " + dispatcherResponse.at("content").at(0)["text"].get<std::string>() + "\n";
                    }
                }
                else
                {
                    results += "Status: SUCCESS\n";
                    if(dispatcherResponse.contains("content") && dispatcherResponse.at("content").is_array() && !dispatcherResponse.at("content").empty() && dispatcherResponse.at("content").at(0).contains("text"))
                    {
                        results += "Result: " + dispatcherResponse["content"][0]["text"].get<std::string>() + "\n";
                    }
                    else
                    {
                        results += "Result: (empty)\n";
                    }
                }
            }
        }
        catch(std::exception const& e)
        {
            allSuccess = false;
            results += "Status: ERROR\n";
            results += "Message: Exception during execution: " + std::string(e.what()) + "\n";
        }
        catch(...)
        {
            allSuccess = false;
            results += "Status: ERROR\n";
            results += "Message: Exception during execution: Unknown\n";
        }
        results += "\n";
    }
    return std::make_pair(allSuccess ? 1 : 2, results + "\nBased on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document.");
}

static std::tuple<juce::Result, common_chat_msg> getAssistantResponse(struct common_sampler* sampler, struct llama_context* context, std::atomic<bool> const& shouldQuit, std::function<void(std::string const&)> callback)
{
    common_chat_msg msg;
    msg.role = "assistant";
    while(true)
    {
        if(shouldQuit.load())
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), msg);
        }

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
        if(callback != nullptr)
        {
            callback(msg.content);
        }

        auto const batch = llama_batch_get_one(&new_token_id, 1);
        auto const ret = llama_decode(context, batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Failed to decode.")), msg);
        }
    }

    return std::make_tuple(juce::Result::ok(), msg);
}

void Application::Neuralyzer::Agent::initialize()
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
                   {
                       MiscDebug("Application::Neuralyzer::Agent", "Global Initialize...");
                       llama_log_set([](enum ggml_log_level, [[maybe_unused]] char const* text, void*)
                                     {
                                         MiscDebug("Application::Neuralyzer::Agent::Init", text);
                                     },
                                     nullptr);
                       llama_backend_init();
                       MiscDebug("Application::Neuralyzer::Agent", "Global Initialized");
                   });
}

void Application::Neuralyzer::Agent::release()
{
    static std::once_flag releaseFlag;
    std::call_once(releaseFlag, []()
                   {
                       MiscDebug("Application::Neuralyzer::Agent", "Global Release...");
                       llama_backend_free();
                       llama_log_set(nullptr, nullptr);
                       MiscDebug("Application::Neuralyzer::Agent", "Global Released");
                   });
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::Agent::getDefaultModelInfo(std::string const& modePath)
{
    auto info = ModelInfo{};
    info.model = juce::File(modePath);
    if(modePath.empty() || !info.model.existsAsFile())
    {
        return info;
    }

    // llama_params_fit is documented as not thread-safe because it touches logger state.
    static std::mutex fitMutex;
    std::unique_lock<std::mutex> lock(fitMutex);

    initialize();

    common_params params;
    params.model.path = modePath;
    params.fit_params_min_ctx = 256;
    info.minP = params.sampling.min_p;
    info.temperature = params.sampling.temp;
    auto mparams = common_model_params_to_llama(params);
    auto cparams = common_context_params_to_llama(params);
    auto const fitStatus = llama_params_fit(modePath.c_str(), &mparams, &cparams,
                                            params.tensor_split,
                                            params.tensor_buft_overrides.data(),
                                            params.fit_params_target.data(),
                                            static_cast<uint32_t>(params.fit_params_min_ctx),
                                            GGML_LOG_LEVEL_ERROR);
    if(fitStatus == LLAMA_PARAMS_FIT_STATUS_ERROR)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to fit parameters for model: " + info.model.getFullPathName());
        return info;
    }
    if(fitStatus == LLAMA_PARAMS_FIT_STATUS_FAILURE)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Unable to fit parameters to device memory for model: " + info.model.getFullPathName());
    }

    auto const minContext = static_cast<uint32_t>(params.fit_params_min_ctx);
    auto constexpr maxInt32 = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    info.contextSize = static_cast<int32_t>(std::max(minContext, std::min(cparams.n_ctx, maxInt32)));
    info.batchSize = static_cast<int32_t>(std::max(minContext, std::min(cparams.n_batch, maxInt32)));

    auto const getMetaFloat = [](llama_model const* model, enum llama_model_meta_key key)
    {
        char buffer[128] = {0};
        if(llama_model_meta_val_str(model, llama_model_meta_key_str(key), buffer, sizeof(buffer)) > 0)
        {
            char* end = nullptr;
            auto const parsed = std::strtof(buffer, &end);
            if(end != nullptr && end != buffer)
            {
                return std::make_optional(parsed);
            }
        }
        return std::optional<float>{};
    };
    auto* model = llama_model_load_from_file(modePath.c_str(), mparams);
    if(model != nullptr)
    {
        info.minP = getMetaFloat(model, LLAMA_MODEL_META_KEY_SAMPLING_MIN_P);
        info.temperature = getMetaFloat(model, LLAMA_MODEL_META_KEY_SAMPLING_TEMP);
        llama_model_free(model);
    }
    else
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load model metadata for sampling defaults: " + info.model.getFullPathName());
    }
    return info;
}

Application::Neuralyzer::Agent::Agent(Mcp::Dispatcher& mcpDispatcher)
: mNeuralyzerMcpDispatcher(mcpDispatcher)
{
}

Application::Neuralyzer::Agent::~Agent()
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
    mMessageEndPositions.clear();
}

Application::Neuralyzer::Mcp::Dispatcher& Application::Neuralyzer::Agent::getMcpDispatcher()
{
    return mNeuralyzerMcpDispatcher;
}

juce::Result Application::Neuralyzer::Agent::initialize(ModelInfo info)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::Agent", "Initialize...");

    mInitResult.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
    mMessageEndPositions.clear();

    if(info.model == juce::File())
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model file is not set.");
        return juce::Result::fail(juce::translate("The model file is not set."));
    }
    if(!info.model.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model file does not exist: " + info.model.getFullPathName());
        return juce::Result::fail(juce::translate("The model file does not exist: FLNAME").replace("FLNAME", info.model.getFullPathName()));
    }

    // Configure common_params for model and context initialization
    common_params params;
    params.use_jinja = true;
    params.model.path = info.model.getFullPathName().toStdString();
    params.chat_template = info.tplt.getFullPathName().toStdString();
    if(info.contextSize.has_value())
    {
        params.n_ctx = info.contextSize.value();
    }
    if(info.batchSize.has_value())
    {
        params.n_batch = info.batchSize.value();
    }
    if(info.minP.has_value())
    {
        params.sampling.min_p = info.minP.value();
    }
    if(info.temperature.has_value())
    {
        params.sampling.temp = info.temperature.value();
    }
    params.fit_params_min_ctx = 256; // Enable fit params for all context sizes above 512 tokens
    params.load_progress_callback_user_data = static_cast<void*>(this);
    params.load_progress_callback = [](float, void* data) -> bool
    {
        return !reinterpret_cast<Agent*>(data)->mShouldQuit.load();
    };

    // Initialize model, context and sampler as a single lifetime-managed object.
    mInitResult = common_init_from_params(params);
    if(mInitResult == nullptr || mInitResult->model() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load model from: " + info.model.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.model.getFullPathName()));
    }
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    llama_set_abort_callback(mInitResult->context(), [](void* data)
                             {
                                 return reinterpret_cast<Agent*>(data)->mShouldQuit.load();
                             },
                             static_cast<void*>(this));

    auto const* context = mInitResult->context();
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    auto const batchCapacity = static_cast<size_t>(llama_n_batch(context));

    MiscDebug("Application::Neuralyzer::Agent", "Model, context, and sampler loaded successfully with context size: " + juce::String(ctxCapacity) + " and batch size: " + juce::String(batchCapacity));

    // Initialize chat templates with Jinja support
    if(!params.chat_template.empty() && !common_chat_verify_template(params.chat_template, true))
    {
        MiscDebug("Application::Neuralyzer::Agent", "The chat template is not supported: " + info.tplt.getFullPathName());
        return juce::Result::fail(juce::translate("The chat template is not supported: FLNAME").replace("FLNAME", info.tplt.getFullPathName()));
    }
    try
    {
        mChatTemplates = common_chat_templates_init(mInitResult->model(), params.chat_template);
    }
    catch(...)
    {
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::Agent", "Failed to initialize chat templates: fallback to default.");
        mChatTemplates = common_chat_templates_init(mInitResult->model(), std::string{});
    }
    if(mChatTemplates == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to initialize chat templates.");
        return juce::Result::fail(juce::translate("Failed to initialize chat templates"));
    }
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    mChatInputs.use_jinja = true;
    mChatInputs.reasoning_format = COMMON_REASONING_FORMAT_DEEPSEEK;
    mChatInputs.enable_thinking = false;
    auto const* vocab = llama_model_get_vocab(mInitResult->model());
    mChatInputs.add_bos = llama_vocab_get_add_bos(vocab);
    mChatInputs.add_eos = llama_vocab_get_add_eos(vocab);
    // Build tools from MCP
    {
        nlohmann::json request;
        request["method"] = "tools/list";
        auto const response = mNeuralyzerMcpDispatcher.handle(request);
        if(!response.contains("tools") || !response.at("tools").is_array())
        {
            MiscDebug("Application::Neuralyzer::Agent", juce::String("Parsing error: ") + response.dump());
            return juce::Result::fail(juce::translate("Failed to retrieve the tools list."));
        }
        for(auto const& toolJson : response.at("tools"))
        {
            if(!toolJson.contains("name") || !toolJson.contains("description"))
            {
                return juce::Result::fail(juce::translate("Invalid tool format received from MCP."));
            }
            common_chat_tool tool;
            tool.name = toolJson.at("name").get<std::string>();
            tool.description = toolJson.at("description").get<std::string>();
            tool.parameters = toolJson.contains("inputSchema") ? toolJson.at("inputSchema").dump() : std::string({});
            MiscDebug("Application::Neuralyzer::Agent", juce::String("Added tool: ") + tool.name);
            mChatInputs.tools.push_back(std::move(tool));
        }
    }
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    MiscDebug("Application::Neuralyzer::Agent", "Successfully initialized model: " + info.model.getFullPathName());
    return juce::Result::ok();
}

std::tuple<juce::Result, std::string, common_chat_params> Application::Neuralyzer::Agent::performQuery(std::string const& role, std::string query, bool allowTools)
{
    auto const createResults = [](juce::Result result, std::string message = {}, common_chat_params params = {})
    {
        return std::make_tuple(std::move(result), std::move(message), std::move(params));
    };

    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    auto* sampler = mInitResult != nullptr ? mInitResult->sampler(0) : nullptr;
    if(context == nullptr || sampler == nullptr)
    {
        return createResults(juce::Result::fail(juce::translate("The model is not initialized.")));
    }

    common_chat_msg userMsg;
    userMsg.role = role;
    userMsg.content = std::move(query);
    mChatInputs.messages.push_back(userMsg);
    mChatInputs.add_generation_prompt = true;
    mChatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_REQUIRED : COMMON_CHAT_TOOL_CHOICE_NONE; // For tool calling if required
    MiscDebug("Application::Neuralyzer::Agent", role + ": " + mChatInputs.messages.back().content);

    common_chat_params params;
    try
    {
        if(mChatInputs.messages.size() == 1)
        {
            params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
        }
        else
        {
            // Do not use full chat inputs
            common_chat_templates_inputs chatInputs;
            chatInputs.use_jinja = mChatInputs.use_jinja;
            chatInputs.reasoning_format = mChatInputs.reasoning_format;
            chatInputs.enable_thinking = mChatInputs.enable_thinking;
            chatInputs.add_generation_prompt = mChatInputs.add_generation_prompt;
            chatInputs.tool_choice = mChatInputs.tool_choice;
            chatInputs.add_bos = mChatInputs.add_bos;
            chatInputs.add_eos = mChatInputs.add_eos;
            chatInputs.messages.push_back(userMsg);
            params = common_chat_templates_apply(mChatTemplates.get(), chatInputs);
        }
    }
    catch(std::exception const& e)
    {
        mChatInputs.messages.pop_back();
        MiscDebug("Application::Neuralyzer::Agent", e.what());
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: ") + e.what()));
    }
    catch(...)
    {
        mChatInputs.messages.pop_back();
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: Unknown error")));
    }

    MiscDebug("Application::Neuralyzer::Agent", "Prompt: " + juce::String(params.prompt).replace("\n", "\\n"));
    auto tokens = common_tokenize(context, params.prompt, true, true);
    // Store the formatted prompt for delta extraction

    // Check if prompt tokens will exceed context capacity
    auto const ctxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    MiscWeakAssert(ctxUsed + tokens.size() <= ctxCapacity);
    if(ctxUsed + tokens.size() > ctxCapacity)
    {
        // Context would overflow - rollback and report error
        mChatInputs.messages.pop_back();
        MiscDebug("Application::Neuralyzer::Agent", "Context memory exceeded.");
        return createResults(juce::Result::fail(juce::translate("Context memory exceeded. If the prompt has been generated using tools, adjust the tool usage settings or start a new conversation.")));
    }
    auto const batchSize = static_cast<size_t>(llama_n_batch(context));

    if(mShouldQuit.load())
    {
        // CRITICAL: Rollback the message addition since we won't generate a response
        mChatInputs.messages.pop_back();
        return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
    }

    auto const promptStartPos = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    // Decode the prompt in chunks so prompts larger than n_batch still work.
    auto position = 0_z;
    while(position < tokens.size())
    {
        if(mShouldQuit.load())
        {
            // Abort before prompt was decoded - just rollback the message
            mChatInputs.messages.pop_back();
            llama_memory_seq_rm(llama_get_memory(context), 0, promptStartPos, -1);
            MiscDebug("Application::Neuralyzer::Agent", "Prompt decoding aborted");
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
        }

        auto const size = std::min(batchSize, tokens.size() - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
        auto const ret = llama_decode(context, batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            // Abort before prompt was decoded - just rollback the message
            mChatInputs.messages.pop_back();
            llama_memory_seq_rm(llama_get_memory(context), 0, promptStartPos, -1);
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")));
        }
        position += size;
    }
    mMessageEndPositions.push_back(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);

    auto const result = getAssistantResponse(sampler, context, mShouldQuit, [&](std::string const& partialResponse)
                                             {
                                                 std::unique_lock<std::mutex> lock(mTempResponseMutex);
                                                 mTempResponse = partialResponse;
                                             });
    mMessageEndPositions.push_back(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
    if(std::get<0>(result).failed())
    {
        return createResults(std::get<0>(result), std::get<1>(result).content);
    }

    MiscDebug("Application::Neuralyzer::Agent", std::get<1>(result).content);

    // Update the stored prompt for next iteration
    mChatInputs.add_generation_prompt = false;
    mChatInputs.messages.push_back(std::get<1>(result));
    mChatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_AUTO : COMMON_CHAT_TOOL_CHOICE_NONE; // Allow tool calling if model decides
    params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);

    return createResults(juce::Result::ok(), std::get<1>(result).content, std::move(params));
}

std::tuple<juce::Result, std::string> Application::Neuralyzer::Agent::sendUserQuery(juce::String const& prompt, bool allowTools)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse.clear();
    }

    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), std::string{});
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Agent loop: keep calling tools until Llama provides a final answer
    static auto constexpr currentRole = "user";
    auto currentQuery = prompt.toStdString();
    std::string finalResponse;
    auto numToolCallsErrors = 0;
    auto numPerformErrors = 0;
    static auto constexpr maxIterations = 25;     // Prevent infinite loops
    static auto constexpr maxPerformErrors = 3;   // Abort if too many tool call parsing errors
    static auto constexpr maxToolCallsErrors = 3; // Abort if too many tool call parsing errors
    for(int iteration = 0; iteration < maxIterations && mShouldQuit.load() == false; ++iteration)
    {
        // Manage context size before each iteration to prevent overflow
        auto ctxResult = manageContextSize();
        if(ctxResult.failed())
        {
            MiscDebug("Application::Neuralyzer::Agent", "Context management warning: " + ctxResult.getErrorMessage().toStdString());
        }

        auto const result = performQuery(currentRole, currentQuery, allowTools);
        if(std::get<0>(result).failed())
        {
            finalResponse = std::get<1>(result);
            currentQuery = std::get<0>(result).getErrorMessage().toStdString();
            if(++numPerformErrors == maxPerformErrors)
            {
                return std::make_tuple(std::get<0>(result), std::get<1>(result));
            }
        }
        else
        {
            numPerformErrors = 0;
            finalResponse = std::get<1>(result);
            auto const toolCallsResult = parseToolCalls(mNeuralyzerMcpDispatcher, std::get<2>(result), finalResponse);
            if(toolCallsResult.first == 0) // No tools called, final answer provided
            {
                return std::make_tuple(juce::Result::ok(), std::get<1>(toolCallsResult));
            }

            currentQuery = std::get<1>(toolCallsResult);
            if(toolCallsResult.first == 1) // Tools call are ok, use the response for a new query
            {
                numToolCallsErrors = 0;
            }
            else if(++numToolCallsErrors >= maxToolCallsErrors) // Too many errors
            {
                MiscDebug("Application::Neuralyzer::Agent", "Too many tool call errors, aborting.");
                return std::make_tuple(juce::Result::fail(juce::translate("Too many tool call errors, aborting.")), finalResponse);
            }
            else if(numToolCallsErrors == maxToolCallsErrors - 1) // Soon too many errors
            {
                currentQuery += "\nWarning: The previous tool calls had errors. Please be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps.";
            }
        }
    }
    if(mShouldQuit.load())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), finalResponse);
    }

    // Max iterations reached - return the last response
    return std::make_tuple(juce::Result::fail(juce::translate("Maximum number of iterations reached.")), finalResponse);
}

juce::Result Application::Neuralyzer::Agent::loadState(juce::File state)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto* context = mInitResult->context();

    if(!state.existsAsFile())
    {
        return juce::Result::fail(juce::translate("The state file does not exist: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    size_t nloaded = 0;
    auto const numCtx = llama_n_ctx(context);
    std::vector<llama_token> array(numCtx);
    if(!llama_state_load_file(context, state.getFullPathName().toRawUTF8(), array.data(), array.size(), &nloaded))
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load state from file: " + state.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load state from file: FLNAME").replace("FLNAME", state.getFullPathName()));
    }
    if(nloaded == 0)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load state from file (no tokens loaded): " + state.getFullPathName());
    }
    MiscDebug("Application::Neuralyzer::Agent", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + state.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::saveState(juce::File state)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto* context = mInitResult->context();

    if(state == juce::File())
    {
        return juce::Result::fail(juce::translate("The state file is not set."));
    }

    // Ensure directory exists
    auto const parentDir = state.getParentDirectory();
    if(!parentDir.exists())
    {
        if(!parentDir.createDirectory())
        {
            MiscDebug("Application::Neuralyzer::Agent", "Failed to create directory: " + parentDir.getFullPathName());
            return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", parentDir.getFullPathName()));
        }
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Save KV cache state. We don't persist prompt tokens here (nullptr, 0) since
    // the KV cache is sufficient for restoring the model state.
    auto const saved = llama_state_save_file(context, state.getFullPathName().toRawUTF8(), nullptr, 0);
    if(!saved)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state to file: " + state.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Saved KV cache to state file: ") + state.getFullPathName());
    return juce::Result::ok();
}

void Application::Neuralyzer::Agent::resetState()
{
    // Clear chat history and KV cache from system prompt onwards
    std::unique_lock<std::mutex> callLock(mCallMutex);
    MiscWeakAssert(!mMessageEndPositions.empty());
    if(mMessageEndPositions.empty())
    {
        return;
    }
    pruneMessagesFromContext({1, mMessageEndPositions.back()});
}

void Application::Neuralyzer::Agent::pruneMessagesFromContext(juce::Range<int32_t> messageRange)
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return;
    }
    auto* context = mInitResult->context();
    MiscWeakAssert(messageRange.getStart() >= 1 && static_cast<size_t>(messageRange.getEnd()) <= mMessageEndPositions.size());
    messageRange = messageRange.getIntersectionWith(juce::Range<int32_t>(1, static_cast<int32_t>(mMessageEndPositions.size())));
    if(messageRange.isEmpty())
    {
        return;
    }
    auto const currentPos = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    // Token range: from end of previous message to end of last message in range
    auto const rangeStartPos = mMessageEndPositions.at(static_cast<size_t>(messageRange.getStart() - 1));
    MiscWeakAssert(rangeStartPos < currentPos);
    if(rangeStartPos >= currentPos)
    {
        return;
    }
    auto const rangeEndPos = std::min(mMessageEndPositions.at(static_cast<size_t>(messageRange.getEnd() - 1)), currentPos);
    auto const shiftPos = rangeEndPos - rangeStartPos;

    // Remove KV cache for pruned messages and shift remaining tokens
    llama_memory_seq_rm(llama_get_memory(context), 0, rangeStartPos, rangeEndPos);
    llama_memory_seq_add(llama_get_memory(context), 0, rangeEndPos, currentPos, -shiftPos);

    // Update message end positions
    auto const posStartIt = std::next(mMessageEndPositions.begin(), messageRange.getStart());
    auto const posEndIt = std::next(mMessageEndPositions.begin(), messageRange.getEnd());
    for(auto posIt = mMessageEndPositions.erase(posStartIt, posEndIt); posIt != mMessageEndPositions.end(); ++posIt)
    {
        *posIt -= shiftPos;
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Shifted context by ") + juce::String(shiftPos) + " tokens.");
}

juce::Result Application::Neuralyzer::Agent::manageContextSize()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return juce::Result::ok();
    }
    auto* context = mInitResult->context();

    auto numCtxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1);
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(context));
    auto const batchSize = static_cast<size_t>(llama_n_batch(context));
    if(ctxCapacity >= numCtxUsed + batchSize)
    {
        return juce::Result::ok();
    }
    auto numToPrune = 0_z;
    auto const numMessages = mChatInputs.messages.size();
    while(numToPrune < numMessages && ctxCapacity < numCtxUsed - static_cast<size_t>(mMessageEndPositions.at(numToPrune++)) + batchSize)
    {
    }
    static auto constexpr maxRecentMessagesToKeep = 3_z; // Keep maximum last N turns
    numToPrune = std::max(numToPrune, numMessages - maxRecentMessagesToKeep);

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Context nearing capacity."));
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse = "Summarizing previous messages to manage context size...\n";
    }

    // The model already has the conversation [1..compressUpTo] in KV cache
    // Just ask it to summarize what it knows - no need to re-encode the conversation
    auto const summaryPrompt = "Summarize our conversation so far in 2-3 sentences maximum, focus on key points and decisions made, don't save the tool results.";
    auto const result = performQuery("user", summaryPrompt, false);
    if(std::get<0>(result).failed())
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Failed to get summary: ") + std::get<0>(result).getErrorMessage());
    }
    pruneMessagesFromContext({1, static_cast<llama_pos>(numToPrune)});
    MiscDebug("Application::Neuralyzer::Agent", "Compressed messages up to index " + juce::String(numToPrune) + ".");
    return std::get<0>(result);
}

juce::String Application::Neuralyzer::Agent::getTemporaryResponse() const
{
    auto response = [this]
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex, std::try_to_lock);
        if(lock.owns_lock())
        {
            return mTempResponse;
        }
        return std::string{};
    }();
    return response;
}

void Application::Neuralyzer::Agent::clearTemporaryResponse()
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    std::unique_lock<std::mutex> lock(mTempResponseMutex);
    mTempResponse.clear();
}

void Application::Neuralyzer::Agent::setShouldQuit(bool state)
{
    mShouldQuit.store(state);
}

bool Application::Neuralyzer::Agent::shouldQuit() const
{
    return mShouldQuit.load();
}

ANALYSE_FILE_END
