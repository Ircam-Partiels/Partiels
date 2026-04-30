#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerTools.h"

ANALYSE_FILE_BEGIN

static std::atomic<bool> sBackendInitialized{false};

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_WARN)
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::CharPointer_UTF8(text));
    }
}

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
                       sBackendInitialized.store(true);
                       MiscDebug("Application::Neuralyzer::Agent", "Global Initialized");
                   });
}

void Application::Neuralyzer::Agent::release()
{
    static std::once_flag releaseFlag;
    std::call_once(releaseFlag, []()
                   {
                       if(!sBackendInitialized.load())
                       {
                           return;
                       }
                       MiscDebug("Application::Neuralyzer::Agent", "Global Release...");
                       llama_backend_free();
                       llama_log_set(nullptr, nullptr);
                       MiscDebug("Application::Neuralyzer::Agent", "Global Released");
                   });
}

Application::Neuralyzer::Agent::Agent(Mcp::Dispatcher& mcpDispatcher)
: mMcpDispatcher(mcpDispatcher)
{
}

Application::Neuralyzer::Agent::~Agent()
{
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
}

void Application::Neuralyzer::Agent::setNotifyCallback(std::function<void()> callback)
{
    std::unique_lock<std::mutex> lock(mNotifyMutex);
    mNotifyCallback = std::move(callback);
}

void Application::Neuralyzer::Agent::notifyStateChanged()
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

Application::Neuralyzer::Mcp::Dispatcher& Application::Neuralyzer::Agent::getMcpDispatcher()
{
    return mMcpDispatcher;
}

void Application::Neuralyzer::Agent::setInstructions(juce::String const& instructions, juce::String const& firstQuery)
{
    std::unique_lock<std::mutex> instructionsLock(mInstructionsMutex);
    mInstructions = instructions;
    mFirstQuery = firstQuery;
}

juce::String Application::Neuralyzer::Agent::getInstructions() const
{
    std::unique_lock<std::mutex> instructionsLock(mInstructionsMutex);
    return mInstructions;
}

juce::String Application::Neuralyzer::Agent::getFirstQuery() const
{
    std::unique_lock<std::mutex> instructionsLock(mInstructionsMutex);
    return mFirstQuery;
}

void Application::Neuralyzer::Agent::setSessionFiles(juce::File const& contextFile, juce::File const& messageFile)
{
    std::unique_lock<std::mutex> sessionLock(mSessionMutex);
    mSessionContextFile = contextFile;
    mSessionMessageFile = messageFile;
}

std::pair<juce::File, juce::File> Application::Neuralyzer::Agent::getSessionFiles() const
{
    std::unique_lock<std::mutex> sessionLock(mSessionMutex);
    return std::make_pair(mSessionContextFile, mSessionMessageFile);
}

juce::Result Application::Neuralyzer::Agent::initializeModel(ModelInfo const& info)
{
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::Agent", "Initialize...");

    mInitResult.reset();
    mChatTemplates = nullptr;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs = common_chat_templates_inputs{};
        notifyStateChanged();
    }
    mContextMemoryUsage.store(0.0f);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = {};
    }

    if(info.modelFile == juce::File())
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model file is not set.");
        return juce::Result::fail(juce::translate("The model file is not set."));
    }
    if(!info.modelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model file does not exist: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("The model file does not exist: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
    }

    // Configure common_params for model and context initialization
    common_params params;
    params.model.path = info.modelFile.getFullPathName().toStdString();
    auto const templateFile = info.modelFile.withFileExtension(".jinja");
    params.chat_template = templateFile.loadFileAsString().toStdString();
    params.use_jinja = !params.chat_template.empty();
    params.n_ctx = info.contextSize.value_or(params.n_ctx);
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
        return !reinterpret_cast<Agent*>(data)->mShouldQuit.load();
    };

    // Initialize model, context and sampler as a single lifetime-managed object.
    mInitResult = common_init_from_params(params);
    if(mShouldQuit.load())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Model loading aborted by user.");
        return juce::Result::fail(juce::translate("Model loading aborted by user."));
    }
    if(mInitResult == nullptr || mInitResult->model() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load model from: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
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
        MiscDebug("Application::Neuralyzer::Agent", "The chat template is not supported: " + templateFile.getFullPathName());
        return juce::Result::fail(juce::translate("The chat template is not supported: FLNAME").replace("FLNAME", templateFile.getFullPathName()));
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

        nlohmann::json request;
        request["method"] = "tools/list";
        auto const response = mMcpDispatcher.handle(request);
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

    MiscDebug("Application::Neuralyzer::Agent", "Successfully initialized model: " + info.modelFile.getFullPathName());
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo.modelFile = info.modelFile;
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

std::tuple<juce::Result, std::string, common_chat_params> Application::Neuralyzer::Agent::performMessage(std::string const& role, std::string query, bool allowTools)
{
    auto const createResults = [](juce::Result result, std::string message = {}, common_chat_params params = {})
    {
        return std::make_tuple(std::move(result), std::move(message), std::move(params));
    };

    auto* context = mInitResult != nullptr ? mInitResult->context() : nullptr;
    auto* sampler = mInitResult != nullptr ? mInitResult->sampler(0) : nullptr;
    MiscStrongAssert(context != nullptr && sampler != nullptr);

    MiscDebug("Application::Neuralyzer::Agent", role + ": " + query);

    common_chat_params params;
    common_chat_templates_inputs chatInputs;
    std::optional<common_chat_msg> preSystemMsg;
    chatInputs.add_generation_prompt = true;
    // REQUIRED forces the model to call a tool on this turn (local chatInputs copy only,
    // does not affect mChatInputs). The end-of-turn value written to mChatInputs below
    // is AUTO, so these two assignments intentionally operate on different objects.
    chatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_REQUIRED : COMMON_CHAT_TOOL_CHOICE_NONE;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        chatInputs.use_jinja = mChatInputs.use_jinja;
        chatInputs.reasoning_format = mChatInputs.reasoning_format;
        chatInputs.enable_thinking = mChatInputs.enable_thinking;
        chatInputs.parallel_tool_calls = mChatInputs.parallel_tool_calls;
        chatInputs.add_bos = mChatInputs.add_bos;
        chatInputs.add_eos = mChatInputs.add_eos;
        if(mChatInputs.messages.empty())
        {
            // If there are no messages in the history, we add the system prompt from
            // the instructions before applying templates. If there are already messages
            // in the history, we assume the system prompt is already included and do
            // not add it again. Tools are only included on the first message to avoid
            // wasting context tokens on subsequent iterations.
            chatInputs.tools = mChatInputs.tools;
            lock.unlock();
            common_chat_msg systemMsg;
            systemMsg.role = "system";
            systemMsg.content = getInstructions().toStdString();
            chatInputs.messages.push_back(systemMsg);
            preSystemMsg = std::move(systemMsg);
        }
    }
    // Some templates expect the user message to be the last one in the history,
    // so we add it to the inputs before applying templates, and remove it afterwards
    // if it was not originally there. We will add the user message back to the
    // history after decoding the prompt so that it's included in the history for the
    // generation and tool calling.
    common_chat_msg userMsg;
    userMsg.role = "user";
    userMsg.content = std::move(query);
    chatInputs.messages.push_back(userMsg);
    try
    {
        params = common_chat_templates_apply(mChatTemplates.get(), chatInputs);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::String(juce::CharPointer_UTF8(e.what())));
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: ") + e.what()));
    }
    catch(...)
    {
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: Unknown error")));
    }

    MiscDebug("Application::Neuralyzer::Agent", "Prompt: " + juce::String(params.prompt).replace("\n", "\\n"));
    auto tokens = common_tokenize(context, params.prompt, true, true);
    // Store the formatted prompt for delta extraction

    // Check if prompt tokens will exceed context capacity
    auto const spaceResults = manageContentMemory(tokens.size());
    if(spaceResults.failed())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Context memory exceeded.");
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
        if(mShouldQuit.load())
        {
            MiscDebug("Application::Neuralyzer::Agent", "Prompt decoding aborted");
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")));
        }

        auto const size = std::min(batchSize, tokens.size() - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
        auto const ret = llama_decode(context, batch);
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
        if(preSystemMsg.has_value())
        {
            mChatInputs.messages.push_back(preSystemMsg.value());
        }
        userMsg.role = role;
        mChatInputs.messages.push_back(userMsg);
        notifyStateChanged();
    }

    // Get the assistant response with streaming support.
    common_chat_msg msg;
    msg.role = "assistant";
    while(true)
    {
        if(mShouldQuit.load())
        {
            return createResults(juce::Result::fail(juce::translate("Operation aborted.")), msg.content);
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
        {
            // Write to mTempResponse only — do NOT call notifyStateChanged() here.
            // The UI polls getTemporaryResponse() via a timer; calling notifyStateChanged()
            // per token would flood the message queue with change notifications.
            std::unique_lock<std::mutex> lock(mTemporaryMutex);
            mTempResponse = msg.content;
        }

        auto const batch = llama_batch_get_one(&new_token_id, 1);
        auto const ret = llama_decode(context, batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")), msg.content);
        }
    }
    MiscDebug("Application::Neuralyzer::Agent", msg.content);

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

    return createResults(juce::Result::ok(), msg.content, std::move(params));
}

juce::Result Application::Neuralyzer::Agent::performQuery(juce::String const& prompt, bool allowTools)
{
    {
        std::unique_lock<std::mutex> lock(mTemporaryMutex);
        mTempResponse.clear();
    }

    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Agent loop: keep calling tools until Llama provides a final answer

    auto currentQuery = prompt.toStdString();
    auto numErrors = 0;
    static auto constexpr maxIterations = 25; // Prevent infinite loops
    static auto constexpr maxErrors = 3;      // Abort if too perform errors

    auto* context = mInitResult->context();
    auto const numRequiredSpace = static_cast<size_t>(llama_n_ctx(context) / 4);
    auto const* role = "user";
    for(int iteration = 0; iteration < maxIterations && mShouldQuit.load() == false; ++iteration)
    {
        // Manage context size before each iteration to prevent overflow
        auto ctxResult = manageContentMemory(numRequiredSpace);
        if(ctxResult.failed())
        {
            MiscDebug("Application::Neuralyzer::Agent", "Context management warning: " + ctxResult.getErrorMessage().toStdString());
        }
        auto const result = performMessage(role, currentQuery, allowTools);
        updateContextMemoryUsage();
        if(std::get<0>(result).failed())
        {
            role = "system";
            currentQuery = std::get<0>(result).getErrorMessage().toStdString();
            if(++numErrors >= maxErrors)
            {
                return std::get<0>(result);
            }
        }
        else
        {
            role = "tool";
            auto const tpResult = Tools::parse(std::get<2>(result), std::get<1>(result));
            if(tpResult.first.wasOk() && tpResult.second.empty())
            {
                return juce::Result::ok();
            }
            else if(tpResult.first.wasOk())
            {
                auto const tcResult = Tools::call(mMcpDispatcher, tpResult.second);
                currentQuery = tcResult.second + "\n\n";
                if(tcResult.first.wasOk())
                {
                    numErrors = 0;
                    currentQuery += "Based on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document.";
                }
                else
                {
                    ++numErrors;
                    currentQuery += "ERROR: " + tcResult.first.getErrorMessage().toStdString() + "\n\n";
                    if(numErrors < maxErrors - 1)
                    {
                        currentQuery += "Warning: The previous tool call had error. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. Ensure the request is consistent with the actual state of the document";
                    }
                    else if(numErrors == maxErrors - 1)
                    {
                        allowTools = false;
                        currentQuery += "Warning: The previous tool calls had too many errors. Tools will be disabled for the next iteration. Please provide a final answer without calling tools.";
                    }
                    else
                    {
                        return juce::Result::fail(juce::translate("Tool call failed: ") + tcResult.first.getErrorMessage());
                    }
                }
            }
            else
            {
                ++numErrors;
                currentQuery += "ERROR: " + tpResult.first.getErrorMessage().toStdString() + "\n\n";
                if(numErrors < maxErrors - 1)
                {
                    currentQuery = "Warning: The previous tool call had error. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. Ensure the tool calls are properly formatted according to the expected structure.";
                }
                else if(numErrors == maxErrors - 1)
                {
                    allowTools = false;
                    currentQuery = "Warning: The previous tool calls had too many errors. Tools will be disabled for the next iteration. Please provide a final answer without calling tools.";
                }
                else
                {
                    return juce::Result::fail(juce::translate("Tool parsing failed: "));
                }
            }
        }
    }
    updateContextMemoryUsage();
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted."));
    }

    // Max iterations reached - return the last response
    return juce::Result::fail(juce::translate("Maximum number of iterations reached."));
}

juce::Result Application::Neuralyzer::Agent::sendQuery(juce::String const& prompt)
{
    return performQuery(prompt, true);
}

juce::Result Application::Neuralyzer::Agent::startSession()
{
    auto resetResult = clearContextMemory();
    return resetResult.wasOk() ? performQuery(getFirstQuery(), false) : resetResult;
}

juce::Result Application::Neuralyzer::Agent::loadSession()
{
    return loadContextFromFile();
}

juce::Result Application::Neuralyzer::Agent::saveSession()
{
    return saveContextToFile();
}

juce::Result Application::Neuralyzer::Agent::loadContextFromFile()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto const [contextFile, messageFile] = getSessionFiles();
    if(messageFile.getSize() <= 0)
    {
        return juce::Result::fail(juce::translate("No session files found."));
    }

    std::vector<common_chat_msg> restoredMessages;
    try
    {
        auto const root = nlohmann::json::parse(messageFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format: FLNAME").replace("FLNAME", messageFile.getFullPathName()));
        }
        for(auto const& messageJson : root.at("messages"))
        {
            common_chat_msg message;
            from_json(messageJson, message);
            restoredMessages.push_back(std::move(message));
        }
    }
    catch(std::exception const& exception)
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Failed to parse message state file: ") + messageFile.getFullPathName() + juce::String(" error: ") + exception.what());
        return juce::Result::fail(juce::translate("Failed to parse message state file: FLNAME").replace("FLNAME", messageFile.getFullPathName()));
    }

    size_t nloaded = 0;
    auto* context = mInitResult->context();

    auto const loadMessages = [&]()
    {
        // No KV cache file: restore the message history and manually integrate it into
        // the context by decoding the full conversation prompt.
        common_chat_templates_inputs chatInputs;
        {
            std::unique_lock<std::mutex> lock(mHistoryMutex);
            chatInputs = mChatInputs;
        }
        chatInputs.add_generation_prompt = false;
        chatInputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_NONE;
        common_chat_params params;
        try
        {
            params = common_chat_templates_apply(mChatTemplates.get(), chatInputs);
        }
        catch(std::exception const& e)
        {
            MiscDebug("Application::Neuralyzer::Agent", juce::String("Failed to apply chat templates when restoring session: ") + e.what());
            return juce::Result::fail(juce::translate("Failed to apply chat templates: ") + e.what());
        }
        auto tokens = common_tokenize(context, params.prompt, true, true);
        auto const batchSize = static_cast<size_t>(llama_n_batch(context));
        auto position = 0_z;
        while(position < tokens.size())
        {
            auto const size = std::min(batchSize, tokens.size() - position);
            auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(size));
            if(llama_decode(context, batch) != 0)
            {
                MiscDebug("Application::Neuralyzer::Agent", "Failed to decode message history into context.");
                return juce::Result::fail(juce::translate("Failed to decode message history into context."));
            }
            position += size;
            updateContextMemoryUsage();
        }
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Restored ") + juce::String(tokens.size()) + juce::String(" tokens from message history: ") + contextFile.getFullPathName());
        return juce::Result::ok();
    };

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages = std::move(restoredMessages);
        notifyStateChanged();
    }

    // The session file was saved with 0 prompt tokens (see saveContextToFile), so
    // n_token_count in the file is always 0 and no token buffer is needed here.
    if(contextFile.getSize() <= 0)
    {
        return loadMessages();
    }
    else if(!llama_state_load_file(context, contextFile.getFullPathName().toRawUTF8(), nullptr, 0, &nloaded))
    {
        return loadMessages();
    }
    if(nloaded == 0)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load state from file (no tokens loaded): " + contextFile.getFullPathName());
    }
    updateContextMemoryUsage();
    MiscDebug("Application::Neuralyzer::Agent", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + contextFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::saveContextToFile()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto [contextFile, messageFile] = getSessionFiles();
    if(contextFile == juce::File{} || messageFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session files found."));
    }

    // Ensure directory exists
    if(!contextFile.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to create directory: " + contextFile.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", contextFile.getParentDirectory().getFullPathName()));
    }
    if(!messageFile.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to create directory: " + messageFile.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", messageFile.getParentDirectory().getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Save KV cache state. We don't persist prompt tokens here (nullptr, 0) since
    // the KV cache is sufficient for restoring the model state.
    auto* context = mInitResult->context();
    auto const saved = llama_state_save_file(context, contextFile.getFullPathName().toRawUTF8(), nullptr, 0);
    if(!saved)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state to file: " + contextFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", contextFile.getFullPathName()));
    }

    nlohmann::json root;
    root["version"] = 1;
    root["messages"] = nlohmann::json::array();
    std::vector<common_chat_msg> messages;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        messages = mChatInputs.messages;
    }
    for(auto const& message : messages)
    {
        nlohmann::json messageJson;
        to_json(messageJson, message);
        root["messages"].push_back(std::move(messageJson));
    }

    if(!messageFile.replaceWithText(juce::String(root.dump(2))))
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state metadata file: " + messageFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state metadata file: FLNAME").replace("FLNAME", messageFile.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Saved KV cache and message history to files: ") + contextFile.getFullPathName() + " " + messageFile.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::clearContextMemory()
{
    // Clear chat history and KV cache from system prompt onwards
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model is not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages.clear();
    }
    notifyStateChanged();
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);
    updateContextMemoryUsage();
    return juce::Result::ok();
}

Application::Neuralyzer::Agent::History Application::Neuralyzer::Agent::getHistory() const
{
    History history;
    std::unique_lock<std::mutex> lock(mHistoryMutex);
    history.reserve(mChatInputs.messages.size());
    for(auto const& message : mChatInputs.messages)
    {
        auto const role = magic_enum::enum_cast<MessageType>(message.role);
        history.push_back({role.value_or(MessageType::assistant), juce::String(message.content)});
    }
    lock.unlock();
    return history;
}

juce::Result Application::Neuralyzer::Agent::manageContentMemory(size_t minNumRequiredTokens)
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

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Context nearing capacity."));

    // Guard: the summary itself needs room to be generated. The target length is
    // ctxCapacity/10 tokens. If less than that is available, performMessage would
    // immediately fail (or recurse), so fall back to a hard clear instead.
    auto const minSpaceForSummary = ctxCapacity / 10;
    if(ctxCapacity - numCtxUsed < minSpaceForSummary)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Context too full to summarize; falling back to hard clear.");
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

void Application::Neuralyzer::Agent::updateContextMemoryUsage()
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

float Application::Neuralyzer::Agent::getContextCapacityUsage() const
{
    return mContextMemoryUsage.load();
}

juce::String Application::Neuralyzer::Agent::getTemporaryResponse() const
{
    std::unique_lock<std::mutex> lock(mTemporaryMutex);
    auto response = mTempResponse;
    lock.unlock();
    return juce::String(response);
}

void Application::Neuralyzer::Agent::setShouldQuit(bool state)
{
    mShouldQuit.store(state);
}

bool Application::Neuralyzer::Agent::shouldQuit() const
{
    return mShouldQuit.load();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::Agent::getModelInfo() const
{
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    return mModelInfo;
}

ANALYSE_FILE_END
