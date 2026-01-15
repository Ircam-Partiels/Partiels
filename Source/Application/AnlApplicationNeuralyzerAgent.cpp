#include "AnlApplicationNeuralyzerAgent.h"

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
                parserParams.parse_tool_calls = true;

                // Use the appropriate parser based on format type
                if(chatParams.format == COMMON_CHAT_FORMAT_PEG_NATIVE || chatParams.format == COMMON_CHAT_FORMAT_PEG_CONSTRUCTED || chatParams.format == COMMON_CHAT_FORMAT_PEG_SIMPLE)
                {
                    // PEG formats require the PEG parser with arena
                    // Load the PEG arena from the saved parser string
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
            return createResults(juce::Result::fail(e.what()));
        }
        catch(...)
        {
            return createResults(juce::Result::fail("Unknown error parsing tool calls"));
        }
    }();

    if(std::get<0>(toolCalls).failed())
    {
        return std::make_pair(2, std::get<0>(toolCalls).getErrorMessage().toStdString() + "\nPlease respond again.");
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

void Application::Neuralyzer::Agent::initialize()
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
                   {
                       MiscDebug("Application::Neuralyzer::Agent", "Initialize...");
                       llama_log_set([](enum ggml_log_level, [[maybe_unused]] char const* text, void*)
                                     {
                                         MiscDebug("Application::Neuralyzer::Agent::Init", text);
                                     },
                                     nullptr);
                       llama_backend_init();
                       MiscDebug("Application::Neuralyzer::Agent", "Initialized");
                   });
}

void Application::Neuralyzer::Agent::release()
{
    static std::once_flag releaseFlag;
    std::call_once(releaseFlag, []()
                   {
                       MiscDebug("Application::Neuralyzer::Agent", "Release...");
                       llama_backend_free();
                       llama_log_set(nullptr, nullptr);
                       MiscDebug("Application::Neuralyzer::Agent", "Released");
                   });
}

Application::Neuralyzer::Agent::Agent(Mcp::Dispatcher& mcpDispatcher)
: mNeuralyzerMcpDispatcher(mcpDispatcher)
{
}

Application::Neuralyzer::Agent::~Agent()
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    llama_log_set(logCallback, nullptr);
    mSampler.reset();
    mContext.reset();
    mModel.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
    mPreviousPromptSize = 0;
    mMessageEndPositions.clear();
}

Application::Neuralyzer::Mcp::Dispatcher& Application::Neuralyzer::Agent::getMcpDispatcher()
{
    return mNeuralyzerMcpDispatcher;
}

juce::Result Application::Neuralyzer::Agent::initialize(ModelInfo info, juce::String const& instruction)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    mSampler.reset();
    mContext.reset();
    mModel.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
    mPreviousPromptSize = 0;
    mMessageEndPositions.clear();

    static int32_t constexpr numGpuLayers = 99;
    static auto constexpr temperature = 0.2f;
    static auto constexpr minP = 0.05f;
    static uint32_t constexpr seed = LLAMA_DEFAULT_SEED;
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

    // Initialize the model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = numGpuLayers;
    model_params.progress_callback = [](float, void* data)
    {
        return !reinterpret_cast<Agent*>(data)->mShouldQuit.load();
    };
    model_params.progress_callback_user_data = static_cast<void*>(this);
    mModel = ModelPtr(llama_model_load_from_file(info.model.getFullPathName().toRawUTF8(), model_params), &llama_model_free);
    if(mModel == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load model from: " + info.model.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.model.getFullPathName()));
    }
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }

    // Initialize the context
    auto ctx_params = llama_context_default_params();
    auto const modelMaxCtx = static_cast<uint32_t>(llama_model_n_ctx_train(mModel.get()));
    ctx_params.n_ctx = std::min(info.contextSize, modelMaxCtx); // Use the smaller of the requested context size and the model's maximum
    ctx_params.n_batch = std::max(info.batchSize, 256u);
    ctx_params.abort_callback = [](void* data)
    {
        return reinterpret_cast<Agent*>(data)->mShouldQuit.load();
    };
    ctx_params.abort_callback_data = static_cast<void*>(this);

    mContext = ContextPtr(llama_init_from_model(mModel.get(), ctx_params), &llama_free);
    if(mContext == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to create llama context");
        return juce::Result::fail(juce::translate("Failed to create llama context"));
    }

    // Initialize the sampler
    mSampler = SamplerPtr(llama_sampler_chain_init(llama_sampler_chain_default_params()), &llama_sampler_free);
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_min_p(minP, 1));
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_dist(seed));
    // Add repetition penalties: last_n=64, repeat=1.15, freq=0.1, presence=0.1
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_penalties(64, 1.15f, 0.1f, 0.1f));

    // Initialize chat templates with Jinja support
    auto const templateOverride = info.tplt.existsAsFile() ? info.tplt.loadFileAsString().toStdString() : std::string{};
    try
    {
        mChatTemplates = common_chat_templates_init(mModel.get(), templateOverride);
    }
    catch(...)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to initialize chat templates: fallback to default.");
        mChatTemplates = common_chat_templates_init(mModel.get(), std::string{});
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
    MiscDebug("Application::Neuralyzer::Agent", "Successfully initialized model: " + info.model.getFullPathName());

    return instruction.isEmpty() ? juce::Result::ok() : sendSystemMessage(instruction.toStdString());
}

juce::Result Application::Neuralyzer::Agent::loadState(juce::File state)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(!state.existsAsFile())
    {
        return juce::Result::fail(juce::translate("The state file does not exist: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    size_t nloaded = 0;
    auto const numCtx = llama_n_ctx(mContext.get());
    std::vector<llama_token> array(numCtx);
    if(!llama_state_load_file(mContext.get(), state.getFullPathName().toRawUTF8(), array.data(), array.size(), &nloaded))
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
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

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
    auto const saved = llama_state_save_file(mContext.get(), state.getFullPathName().toRawUTF8(), nullptr, 0);
    if(!saved)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state to file: " + state.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Saved KV cache to state file: ") + state.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::sendSystemMessage(std::string instruction)
{
    if(instruction.empty())
    {
        MiscDebug("Application::Neuralyzer::Agent", "The instruction content is empty.");
        return juce::Result::fail(juce::translate("The instruction content is empty."));
    }

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
                continue;
            }
            common_chat_tool tool;
            tool.name = toolJson.at("name").get<std::string>();
            tool.description = toolJson.at("description").get<std::string>();
            tool.parameters = toolJson.contains("inputSchema") ? toolJson.at("inputSchema").dump() : "{}";
            MiscDebug("Application::Neuralyzer::Agent", juce::String("Added tool: ") + tool.name);
            mChatInputs.tools.push_back(std::move(tool));
        }
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Add system message to chat history
    common_chat_msg msg;
    msg.role = "system";
    msg.content = std::move(instruction);
    mChatInputs.messages.push_back(std::move(msg));

    MiscDebug("Application::Neuralyzer::Agent", juce::String("System message: ") + instruction);
    MiscDebug("Application::Neuralyzer::Agent",
              juce::String("Registered ") + juce::String(mChatInputs.tools.size()) + juce::String(" tools for this chat"));

    // Generate full prompt and extract just the new portion
    mChatInputs.add_generation_prompt = false;
    mChatInputs.use_jinja = true;

    auto const params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
    // Extract just the new prompt portion (since last message)
    auto const prompt = params.prompt.size() > mPreviousPromptSize ? params.prompt.substr(mPreviousPromptSize) : params.prompt;

    // Tokenize the formatted system message
    auto tokens = common_tokenize(mContext.get(), prompt, false, false);

    // Check if system prompt will fit in context
    MiscWeakAssert(tokens.size() <= static_cast<size_t>(llama_n_ctx(mContext.get())));
    if(tokens.size() > static_cast<size_t>(llama_n_ctx(mContext.get())))
    {
        mChatInputs.messages.pop_back();
        return juce::Result::fail(juce::translate("System prompt exceeds context capacity."));
    }

    // Decode tokens in batches to populate KV cache (no generation/sampling)
    auto position = 0_z;
    auto const numBatch = static_cast<size_t>(llama_n_batch(mContext.get()));
    while(position < std::min(tokens.size(), numBatch))
    {
        if(mShouldQuit.load())
        {
            mChatInputs.messages.pop_back();
            llama_memory_seq_rm(llama_get_memory(mContext.get()), 0, 0, -1);
            return juce::Result::fail(juce::translate("Operation aborted."));
        }
        auto const size = std::min(numBatch, tokens.size() - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
        auto const decodeResult = llama_decode(mContext.get(), batch);
        MiscWeakAssert(decodeResult == 0);
        if(decodeResult != 0)
        {
            mPreviousPromptSize = 0;
            mChatInputs.messages.pop_back();
            llama_memory_seq_rm(llama_get_memory(mContext.get()), 0, 0, -1);
            MiscDebug("Application::Neuralyzer::Agent", "Failed to decode, ret = " + juce::String(decodeResult));
            return juce::Result::fail(juce::translate("Failed to decode ERRORCODE.").replace("ERRORCODE", juce::String(decodeResult)));
        }
        position += size;
    }

    // Store the formatted prompt for delta extraction
    mPreviousPromptSize = params.prompt.size();

    // Track system prompt end position for selective KV cache management
    auto const systemEndPos = llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1;
    mMessageEndPositions.push_back(systemEndPos);

    MiscDebug("Application::Neuralyzer::Agent", juce::String("System prompt ends at position: ") + juce::String(systemEndPos));
    return juce::Result::ok();
}

std::tuple<juce::Result, std::string, common_chat_params> Application::Neuralyzer::Agent::performUserQuery(std::string query, bool allowTools)
{
    auto const createResults = [](juce::Result result, std::string message = {}, common_chat_params params = {})
    {
        return std::make_tuple(std::move(result), std::move(message), std::move(params));
    };

    common_chat_msg userMsg;
    userMsg.role = "user";
    userMsg.content = std::move(query);
    mChatInputs.messages.push_back(std::move(userMsg));
    MiscDebug("Application::Neuralyzer::Agent", juce::String("Message user: ") + mChatInputs.messages.back().content);

    // Generate prompt for the current message
    mChatInputs.add_generation_prompt = true;
    mChatInputs.use_jinja = true;
    mChatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_AUTO : COMMON_CHAT_TOOL_CHOICE_NONE; // Allow tool calling if model decides

    auto const params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
    auto const prompt = params.prompt.size() > mPreviousPromptSize ? params.prompt.substr(mPreviousPromptSize) : params.prompt;

    auto tokens = common_tokenize(mContext.get(), prompt, true, true);

    // Check if prompt tokens will exceed context capacity
    auto const promptTokenCount = tokens.size();
    auto const numCtxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1);
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(mContext.get()));
    MiscWeakAssert(numCtxUsed + promptTokenCount <= ctxCapacity);
    if(numCtxUsed + promptTokenCount > ctxCapacity)
    {
        // Context would overflow - rollback and report error
        mChatInputs.messages.pop_back();
        MiscDebug("Application::Neuralyzer::Agent", "Context memory exceeded.");
        return createResults(juce::Result::fail(juce::translate("Context memory exceeded. If the prompt has been generated using tools, adjust the tool usage settings or start a new conversation.")));
    }
    auto const batchSize = static_cast<size_t>(llama_n_batch(mContext.get()));
    if(tokens.size() > batchSize)
    {
        mChatInputs.messages.pop_back();
        MiscDebug("Application::Neuralyzer::Agent", "Prompt exceeds batch size.");
        return createResults(juce::Result::fail(juce::translate("Prompt exceeds batch size. If the prompt has been generated using tools, adjust the tool usage settings or start a new conversation.")));
    }

    if(mShouldQuit.load())
    {
        // CRITICAL: Rollback the message addition since we won't generate a response
        mChatInputs.messages.pop_back();
        return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
    }

    std::string response;
    bool promptDecoded = false;
    llama_pos userMsgEndPos = 0; // Will be set after prompt is decoded
    auto const finalizeState = [&]()
    {
        if(!promptDecoded)
        {
            // Abort before prompt was decoded - just rollback the message
            mChatInputs.messages.pop_back();
        }
        else if(!response.empty())
        {
            // CRITICAL: Save the partial response to maintain history consistency
            // The KV cache already contains prompt + generated tokens
            common_chat_msg assistantMsg;
            assistantMsg.role = "assistant";
            assistantMsg.content = response;
            mChatInputs.messages.push_back(std::move(assistantMsg));
            mMessageEndPositions.push_back(userMsgEndPos);
            mMessageEndPositions.push_back(llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1);
        }
    };
    // Note: tokens must be non-const for llama_batch_get_one
    auto batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(std::min(tokens.size(), batchSize)));
    while(true)
    {
        if(mShouldQuit.load())
        {
            finalizeState();
            MiscDebug("Application::Neuralyzer::Agent", juce::String("Generation aborted") + (response.empty() ? juce::String{} : juce::String(", saved partial response with ") + juce::String(response.size()) + juce::String(" characters")));
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
        }

        auto const ret = llama_decode(mContext.get(), batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            finalizeState();
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")));
        }
        if(!promptDecoded)
        {
            // Record user message end position (after prompt tokens are decoded)
            promptDecoded = true;
            userMsgEndPos = llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1;
        }

        // Sample the next token
        auto new_token_id = llama_sampler_sample(mSampler.get(), mContext.get(), -1);
        auto const* vocab = llama_model_get_vocab(llama_get_model(mContext.get()));
        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(vocab, new_token_id))
        {
            break;
        }

        auto const piece = [&]()
        {
            // Convert the token to a string and add it to the response
            char buf[1024];
            auto const n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            MiscWeakAssert(n >= 0);
            if(n < 0)
            {
                MiscDebug("Application::Neuralyzer::Agent", "Failed to convert token to piece");
                return std::string{};
            }
            return std::string(buf, static_cast<size_t>(n));
        }();
        response += piece;
        {
            std::unique_lock<std::mutex> lock(mTempResponseMutex);
            mTempResponse = response;
        }

        // Prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    MiscDebug("Application::Neuralyzer::Agent", response);
    finalizeState();

    // Update the stored prompt for next iteration
    mChatInputs.add_generation_prompt = false;
    mChatInputs.use_jinja = true;
    auto finalParams = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
    mPreviousPromptSize = finalParams.prompt.size();

    return createResults(juce::Result::ok(), std::move(response), std::move(finalParams));
}

std::tuple<juce::Result, std::string> Application::Neuralyzer::Agent::sendUserQuery(juce::String const& prompt)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse.clear();
    }

    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), std::string{});
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Agent loop: keep calling tools until Llama provides a final answer
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

        auto const result = performUserQuery(currentQuery, true);
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

void Application::Neuralyzer::Agent::pruneMessagesFromContext(juce::Range<llama_pos> messageRange)
{
    if(mContext == nullptr)
    {
        return;
    }
    MiscWeakAssert(messageRange.getStart() >= 1 && static_cast<size_t>(messageRange.getEnd()) <= mMessageEndPositions.size());
    messageRange = messageRange.getIntersectionWith(juce::Range<llama_pos>(1, static_cast<llama_pos>(mMessageEndPositions.size())));
    if(messageRange.isEmpty())
    {
        return;
    }
    auto const currentPos = llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1;
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
    llama_memory_seq_rm(llama_get_memory(mContext.get()), 0, rangeStartPos, rangeEndPos);
    llama_memory_seq_add(llama_get_memory(mContext.get()), 0, rangeEndPos, currentPos, -shiftPos);

    // Update message end positions
    auto const posStartIt = std::next(mMessageEndPositions.begin(), messageRange.getStart());
    auto const posEndIt = std::next(mMessageEndPositions.begin(), messageRange.getEnd());
    for(auto posIt = mMessageEndPositions.erase(posStartIt, posEndIt); posIt != mMessageEndPositions.end(); ++posIt)
    {
        *posIt -= shiftPos;
    }
    // Erase the pruned messages
    auto const messageStartIt = std::next(mChatInputs.messages.begin(), messageRange.getStart());
    auto const messageEndIt = std::next(mChatInputs.messages.begin(), messageRange.getEnd());
    mChatInputs.messages.erase(messageStartIt, messageEndIt);

    // Recalculate prompt size for remaining messages so delta extraction works correctly
    mChatInputs.add_generation_prompt = false;
    mChatInputs.use_jinja = true;
    auto const params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);
    mPreviousPromptSize = params.prompt.size();

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Shifted context by ") + juce::String(shiftPos) + " tokens.");
    MiscDebug("Application::Neuralyzer::Agent", juce::String("Pruned messages to ") + juce::String(mChatInputs.messages.size()) + " messages.");
}

juce::Result Application::Neuralyzer::Agent::manageContextSize()
{
    if(mContext == nullptr)
    {
        return juce::Result::ok();
    }

    auto numCtxUsed = static_cast<size_t>(llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1);
    auto const ctxCapacity = static_cast<size_t>(llama_n_ctx(mContext.get()));
    auto const batchSize = static_cast<size_t>(llama_n_batch(mContext.get()));
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
    auto const summaryPrompt = "Summarize our conversation so far in 2-3 sentences maximum, focus on key points and decisions made, don't save the tool results:";
    auto const result = performUserQuery(summaryPrompt, false);
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
