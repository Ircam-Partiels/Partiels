#include "AnlApplicationNeuralyzerAgent.h"
#include "AnlApplicationNeuralyzerTools.h"

ANALYSE_FILE_BEGIN

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

Application::Neuralyzer::Agent::Agent(Mcp::Dispatcher& mcpDispatcher)
: mMcpDispatcher(mcpDispatcher)
{
}

Application::Neuralyzer::Agent::~Agent()
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
}

Application::Neuralyzer::Mcp::Dispatcher& Application::Neuralyzer::Agent::getMcpDispatcher()
{
    return mMcpDispatcher;
}

juce::Result Application::Neuralyzer::Agent::initialize(ModelInfo const& info, juce::String const& instructions)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::Agent", "Initialize...");

    mInitResult.reset();
    mChatTemplates = nullptr;
    mChatInputs = common_chat_templates_inputs{};
    mContextCapacityUsage.store(0.0f);
    mModelInfo = {};

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
    params.use_jinja = true;
    params.model.path = info.modelFile.getFullPathName().toStdString();
    auto const templateFile = info.modelFile.withFileExtension(".jinja");
    params.chat_template = templateFile.loadFileAsString().toStdString();
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
    if(mInitResult == nullptr || mInitResult->model() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load model from: " + info.modelFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", info.modelFile.getFullPathName()));
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
    std::lock_guard<std::mutex> lock(mTempResponseMutex);
    mModelInfo.modelFile = info.modelFile;
    mModelInfo.contextSize = ctxCapacity;
    mModelInfo.batchSize = batchCapacity;
    mModelInfo.minP = params.sampling.min_p;
    mModelInfo.temperature = params.sampling.temp;
    mModelInfo.topP = params.sampling.top_p;
    mModelInfo.topK = params.sampling.top_k;
    mModelInfo.presencePenalty = params.sampling.penalty_present;
    mModelInfo.repetitionPenalty = params.sampling.penalty_repeat;
    mInstructions = instructions;
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
    MiscStrongAssert(context != nullptr && sampler != nullptr);

    MiscDebug("Application::Neuralyzer::Agent", role + ": " + query);

    common_chat_params params;
    common_chat_templates_inputs chatInputs;
    chatInputs.add_generation_prompt = true;
    chatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_REQUIRED : COMMON_CHAT_TOOL_CHOICE_NONE;
    chatInputs.use_jinja = mChatInputs.use_jinja;
    chatInputs.reasoning_format = mChatInputs.reasoning_format;
    chatInputs.enable_thinking = mChatInputs.enable_thinking;
    chatInputs.add_bos = mChatInputs.add_bos;
    chatInputs.add_eos = mChatInputs.add_eos;
    std::optional<common_chat_msg> preSystemMsg;
    if(mChatInputs.messages.empty())
    {
        JUCE_COMPILER_WARNING("check if that replaced the original system prompt")
        chatInputs.tools = mChatInputs.tools;
        common_chat_msg systemMsg;
        systemMsg.role = "system";
        systemMsg.content = mInstructions.toStdString();
        chatInputs.messages.push_back(systemMsg);
        preSystemMsg = std::move(systemMsg);
    }
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
    auto const spaceResults = ensureContextSpace(tokens.size());
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
        updateContextCapacity();
    }
    if(preSystemMsg.has_value())
    {
        mChatInputs.messages.push_back(preSystemMsg.value());
    }
    userMsg.role = role;
    mChatInputs.messages.push_back(userMsg);

    // Get the assistant response with streaming support.
    common_chat_msg msg;
    msg.role = "assistant";
    while(true)
    {
        if(mShouldQuit.load())
        {
            // Rollback the user message and position endpoint since this attempt failed
            if(preSystemMsg.has_value())
            {
                mChatInputs.messages.pop_back();
            }
            mChatInputs.messages.pop_back();
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
            std::unique_lock<std::mutex> lock(mTempResponseMutex);
            mTempResponse = msg.content;
        }

        auto const batch = llama_batch_get_one(&new_token_id, 1);
        auto const ret = llama_decode(context, batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            // Rollback the user message and position endpoint since this attempt failed
            if(preSystemMsg.has_value())
            {
                mChatInputs.messages.pop_back();
            }
            mChatInputs.messages.pop_back();
            return createResults(juce::Result::fail(juce::translate("Failed to decode.")), msg.content);
        }
    }
    MiscDebug("Application::Neuralyzer::Agent", msg.content);

    mChatInputs.messages.push_back(msg);

    mChatInputs.add_generation_prompt = false;
    mChatInputs.tool_choice = allowTools ? COMMON_CHAT_TOOL_CHOICE_AUTO : COMMON_CHAT_TOOL_CHOICE_NONE; // Allow tool calling if model decides
    params = common_chat_templates_apply(mChatTemplates.get(), mChatInputs);

    return createResults(juce::Result::ok(), msg.content, std::move(params));
}

std::tuple<juce::Result, std::vector<juce::String>> Application::Neuralyzer::Agent::sendUserQuery(juce::String const& prompt, bool allowTools)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse.clear();
    }

    std::vector<juce::String> finalResponses;
    if(mInitResult == nullptr || mInitResult->model() == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), finalResponses);
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
        auto ctxResult = ensureContextSpace(numRequiredSpace);
        if(ctxResult.failed())
        {
            MiscDebug("Application::Neuralyzer::Agent", "Context management warning: " + ctxResult.getErrorMessage().toStdString());
        }
        auto const result = performQuery(role, currentQuery, allowTools);
        finalResponses.push_back(std::get<1>(result));
        updateContextCapacity();
        if(std::get<0>(result).failed())
        {
            role = "system";
            currentQuery = std::get<0>(result).getErrorMessage().toStdString();
            if(++numErrors >= maxErrors)
            {
                return std::make_tuple(std::get<0>(result), finalResponses);
            }
        }
        else
        {
            role = "tool";
            auto const tpResult = Tools::parse(std::get<2>(result), std::get<1>(result));
            if(tpResult.first.wasOk() && tpResult.second.empty())
            {
                return std::make_tuple(juce::Result::ok(), finalResponses);
            }
            else if(tpResult.first.wasOk())
            {
                auto const tcResult = Tools::call(mMcpDispatcher, tpResult.second);
                if(tcResult.first.wasOk())
                {
                    numErrors = 0;
                    currentQuery = tcResult.second + "\nBased on these results, provide your final answer or call more tools if needed. If necessary, ensure your response is accurate based on the current state of the document.";
                }
                else
                {
                    ++numErrors;
                    if(numErrors < maxErrors - 1)
                    {
                        currentQuery = juce::String("Warning: The previous tool call had error: ERROR. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. Ensure the request is consistent with the actual state of the document").replace("ERROR", tcResult.first.getErrorMessage()).toStdString();
                    }
                    else if(numErrors == maxErrors - 1)
                    {
                        allowTools = false;
                        currentQuery = "Warning: The previous tool calls had too many errors. Tools will be disabled for the next iteration. Please provide a final answer without calling tools.";
                    }
                    else
                    {
                        return std::make_tuple(juce::Result::fail(juce::translate("Tool call failed: ") + tcResult.first.getErrorMessage()), finalResponses);
                    }
                }
            }
            else
            {
                ++numErrors;
                if(numErrors < maxErrors - 1)
                {
                    currentQuery = juce::String("Warning: The previous tool call had error: ERROR. Be careful with further tool calls. Call one tool at a time and split complex tasks into smaller steps. Ensure the tool calls are properly formatted according to the expected structure, the letter case is respected (lower case for JSON boolean), the JSON and/or XML formats are well-formatted.").replace("ERROR", tpResult.first.getErrorMessage()).toStdString();
                }
                else if(numErrors == maxErrors - 1)
                {
                    allowTools = false;
                    currentQuery = "Warning: The previous tool calls had too many errors. Tools will be disabled for the next iteration. Please provide a final answer without calling tools.";
                }
                else
                {
                    return std::make_tuple(juce::Result::fail(juce::translate("Tool parsing failed: ") + tpResult.first.getErrorMessage()), finalResponses);
                }
            }
        }
    }
    updateContextCapacity();
    if(mShouldQuit.load())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), finalResponses);
    }

    // Max iterations reached - return the last response
    return std::make_tuple(juce::Result::fail(juce::translate("Maximum number of iterations reached.")), finalResponses);
}

juce::Result Application::Neuralyzer::Agent::loadState(juce::File contextState, juce::File messageState)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto* context = mInitResult->context();

    if(!contextState.existsAsFile())
    {
        return juce::Result::fail(juce::translate("The context state file does not exist: FLNAME").replace("FLNAME", contextState.getFullPathName()));
    }
    if(!messageState.existsAsFile())
    {
        return juce::Result::fail(juce::translate("The message state file does not exist: FLNAME").replace("FLNAME", messageState.getFullPathName()));
    }

    std::vector<common_chat_msg> restoredMessages;
    try
    {
        auto const root = nlohmann::json::parse(messageState.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format: FLNAME").replace("FLNAME", messageState.getFullPathName()));
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
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Failed to parse message state file: ") + messageState.getFullPathName() + juce::String(" error: ") + exception.what());
        return juce::Result::fail(juce::translate("Failed to parse message state file: FLNAME").replace("FLNAME", messageState.getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    size_t nloaded = 0;
    auto const numCtx = llama_n_ctx(context);
    std::vector<llama_token> array(numCtx);
    if(!llama_state_load_file(context, contextState.getFullPathName().toRawUTF8(), array.data(), array.size(), &nloaded))
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load state from file: " + contextState.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load state from file: FLNAME").replace("FLNAME", contextState.getFullPathName()));
    }
    if(nloaded == 0)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to load state from file (no tokens loaded): " + contextState.getFullPathName());
    }
    mChatInputs.messages = std::move(restoredMessages);
    updateContextCapacity();
    MiscDebug("Application::Neuralyzer::Agent", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + contextState.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::saveState(juce::File contextState, juce::File messageState)
{
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mInitResult == nullptr || mInitResult->context() == nullptr || mInitResult->sampler(0) == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    auto* context = mInitResult->context();
    if(contextState == juce::File())
    {
        return juce::Result::fail(juce::translate("The context state file is not set."));
    }
    if(messageState == juce::File())
    {
        return juce::Result::fail(juce::translate("The message state file is not set."));
    }

    // Ensure directory exists
    if(!contextState.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to create directory: " + contextState.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", contextState.getParentDirectory().getFullPathName()));
    }
    if(!messageState.getParentDirectory().createDirectory())
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to create directory: " + messageState.getParentDirectory().getFullPathName());
        return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", messageState.getParentDirectory().getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    // Save KV cache state. We don't persist prompt tokens here (nullptr, 0) since
    // the KV cache is sufficient for restoring the model state.
    auto const saved = llama_state_save_file(context, contextState.getFullPathName().toRawUTF8(), nullptr, 0);
    if(!saved)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state to file: " + contextState.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", contextState.getFullPathName()));
    }

    nlohmann::json root;
    root["version"] = 1;
    root["messages"] = nlohmann::json::array();
    for(auto const& message : mChatInputs.messages)
    {
        nlohmann::json messageJson;
        to_json(messageJson, message);
        root["messages"].push_back(std::move(messageJson));
    }

    if(!messageState.replaceWithText(juce::String(root.dump(2))))
    {
        MiscDebug("Application::Neuralyzer::Agent", "Failed to save state metadata file: " + messageState.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state metadata file: FLNAME").replace("FLNAME", messageState.getFullPathName()));
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Saved KV cache and message history to files: ") + contextState.getFullPathName() + " " + messageState.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::resetState()
{
    // Clear chat history and KV cache from system prompt onwards
    std::unique_lock<std::mutex> callLock(mCallMutex);
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Agent", "The model is not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }
    MiscWeakAssert(mChatInputs.messages.size() > 2);
    if(mChatInputs.messages.size() <= 2)
    {
        MiscDebug("Application::Neuralyzer::Agent", "Not enough messages in history to reset state.");
        return juce::Result::fail(juce::translate("Not enough messages in history to reset state."));
    }
    MiscStrongAssert(mChatInputs.messages.size() == mChatInputs.messages.size());
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);
    mChatInputs.messages.clear();
    updateContextCapacity();
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::Agent::ensureContextSpace(size_t minNumRequiredTokens)
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
    if(mChatInputs.messages.size() <= 1)
    {
        return juce::Result::fail(juce::translate("Context is nearing capacity and there are no messages to summarize or prune."));
    }

    MiscDebug("Application::Neuralyzer::Agent", juce::String("Context nearing capacity."));
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse = "Summarizing previous messages to manage context size...\n";
    }
    auto const summaryPrompt = juce::String("Summarize our conversation from the 2nd message up to last message in a concise way that preserves important details and context. This summary will replace the earlier messages in the context to free up space, so it should capture key points, decisions made, and any relevant information. The summary should be brief (MAXNUMTOKENS tokens max) but informative, allowing us to continue our discussion without losing important context.").replace("MAXNUMTOKENS", juce::String(ctxCapacity / 10));
    auto const summaryResult = performQuery("user", summaryPrompt.toStdString(), false);
    if(std::get<0>(summaryResult).failed())
    {
        MiscDebug("Application::Neuralyzer::Agent", juce::String("Failed to get summary: ") + std::get<0>(summaryResult).getErrorMessage());
    }
    resetState();
    auto const restoreResult = performQuery("user", std::get<1>(summaryResult), false);
    MiscDebug("Application::Neuralyzer::Agent", "Context management completed. Summary: " + std::get<1>(summaryResult));
    return std::get<0>(restoreResult);
}

void Application::Neuralyzer::Agent::updateContextCapacity()
{
    if(mInitResult == nullptr || mInitResult->context() == nullptr)
    {
        return;
    }
    auto* context = mInitResult->context();
    auto const ctxCapacity = static_cast<llama_pos>(llama_n_ctx(context));
    auto const ctxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    mContextCapacityUsage.store(static_cast<float>(static_cast<double>(ctxUsed) / static_cast<double>(ctxCapacity)));
}

float Application::Neuralyzer::Agent::getContextCapacityUsage() const
{
    return mContextCapacityUsage.load();
}

bool Application::Neuralyzer::Agent::isInitialized() const
{
    return mInitResult != nullptr && mInitResult->context() != nullptr && mInitResult->sampler(0) != nullptr;
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
    std::lock_guard<std::mutex> lock(mTempResponseMutex);
    return mModelInfo;
}

ANALYSE_FILE_END
