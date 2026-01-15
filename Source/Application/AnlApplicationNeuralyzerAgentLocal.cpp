#include "AnlApplicationNeuralyzerAgentLocal.h"
#include "AnlApplicationInstance.h"
#include <AnlNeuralyzerData.h>
#include <TestResultsData.h>

ANALYSE_FILE_BEGIN

static void logCallback(enum ggml_log_level level, [[maybe_unused]] const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_WARN)
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::CharPointer_UTF8(text));
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

juce::File Application::Neuralyzer::AgentLocal::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Models");
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

juce::URL Application::Neuralyzer::AgentLocal::getDefaultModelUrl()
{
    return juce::URL(PARTIELS_NEURALYZER_DEFAULT_MODEL);
}

std::vector<juce::URL> Application::Neuralyzer::AgentLocal::getDefaultModelUrls()
{
    static auto const lines = juce::StringArray::fromLines(juce::String(AnlNeuralyzerData::defaultModels_txt));
    std::vector<juce::URL> urls;
    for(auto const& line : lines)
    {
        auto url = juce::URL(line.trim());
        if(url.isWellFormed())
        {
            urls.push_back(std::move(url));
        }
    }
    return urls;
}

void Application::Neuralyzer::AgentLocal::downloadDefaultModelIfNecessary()
{
    if(!getAvailableModels().empty())
    {
        return;
    }

    auto const callback = [](Downloader::Process const& process)
    {
        if(process.getResult().failed() && !process.getOutputFile().existsAsFile())
        {
            return;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Default Model Downloaded"))
                                 .withMessage(juce::translate("The default model NAME has been successfully downloaded. You can now select it in the model list. Would you like to open the Neuralyzer settings panel to select it now?").replace("NAME", process.getOutputFile().getFileNameWithoutExtension()))
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
    downloader.start(getDefaultModelDirectory(), getDefaultModelUrl(), callback);
}

Application::Neuralyzer::AgentLocal::AgentLocal(Mcp::Dispatcher& mcpDispatcher, Guard& guard, Rag::Engine& rag)
: mMcpDispatcher(mcpDispatcher)
, mGuard(guard)
, mRag(rag)
{
}

Application::Neuralyzer::AgentLocal::~AgentLocal()
{
    llama_log_set(logCallback, nullptr);
    mInitResult.reset();
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

void Application::Neuralyzer::AgentLocal::setFirstQuery(juce::String const& firstQuery)
{
    std::unique_lock<std::mutex> instructionsLock(mInstructionsMutex);
    mFirstQuery = firstQuery;
}

juce::String Application::Neuralyzer::AgentLocal::getFirstQuery() const
{
    std::unique_lock<std::mutex> instructionsLock(mInstructionsMutex);
    return mFirstQuery;
}

juce::Result Application::Neuralyzer::AgentLocal::initializeModel(ModelInfo const& info)
{
    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);
    MiscDebug("Application::Neuralyzer::AgentLocal", "Initialize...");

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
        mChatInputs.tools = mMcpDispatcher.getToolList();
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
            systemMsg.content = mMcpDispatcher.getInstructions();
            chatInputs.messages.push_back(systemMsg);
            preSystemMsg = std::move(systemMsg);
        }
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
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String(juce::CharPointer_UTF8(e.what())));
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: ") + e.what()));
    }
    catch(...)
    {
        return createResults(juce::Result::fail(juce::translate("Failed to apply chat templates: Unknown error")));
    }

    MiscDebug("Application::Neuralyzer::AgentLocal", "Prompt: " + juce::String(params.prompt).replace("\n", "\\n"));
    auto tokens = common_tokenize(context, params.prompt, true, true);
    // Store the formatted prompt for delta extraction

    // Check if prompt tokens will exceed context capacity
    auto const spaceResults = manageContentMemory(tokens.size());
    if(spaceResults.failed())
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Context memory exceeded.");
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
            MiscDebug("Application::Neuralyzer::AgentLocal", "Prompt decoding aborted");
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

    auto* context = mInitResult->context();
    auto const numRequiredSpace = static_cast<size_t>(llama_n_ctx(context) / 4);
    auto const* role = "user";
    for(auto iteration = 0_z; iteration < maxIterations && mShouldQuit.load() == false; ++iteration)
    {
        // Manage context size before each iteration to prevent overflow
        auto ctxResult = manageContentMemory(numRequiredSpace);
        if(ctxResult.failed())
        {
            MiscDebug("Application::Neuralyzer::AgentLocal", "Context management warning: " + ctxResult.getErrorMessage().toStdString());
        }

        // Perform a message exchange with the model, get the assistant response
        // and any chat parameters (e.g. tools to call). If the model failed to
        // generate a response, we treat it as a perform error and provide the error
        // message as feedback to the model.
        auto const [performResult, assistantMsg, chatParams] = performMessage(role, currentQuery, allowTools);
        if(performResult.failed())
        {
            if(++numErrors >= maxErrors)
            {
                // The model failed to generate a response for too many consecutive
                // iterations, we abort the entire query.
                return performResult;
            }
            // The model failed to generate a response, we treat it as a perform
            // error and provide the error message as feedback to the model.
            role = "system";
            currentQuery = performResult.getErrorMessage().toStdString();
        }
        else if(assistantMsg.empty())
        {
            if(++numErrors >= maxErrors)
            {
                // The model failed to generate a response for too many consecutive
                // iterations, we abort the entire query.
                return juce::Result::fail(juce::translate("The model did not return any response."));
            }
            // The model failed to generate a response, we treat it as a perform
            // error and provide the error message as feedback to the model.
            role = "system";
            currentQuery = "The model did not return any response.";
        }
        else
        {
            role = "tool";
            // The model successfully generated a response, we parse the response for
            // tool calls and either call the tools or return the final answer.
            common_chat_parser_params parserParams(chatParams);
            if(!chatParams.parser.empty())
            {
                parserParams.parser.load(nlohmann::json::parse(chatParams.parser).dump());
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
                    return std::make_tuple(std::string("Exception during tool call parsing: ") + e.what(), std::vector<common_chat_tool_call>{});
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
                    return juce::Result::fail(juce::translate("Tool call failed: ERROR").replace("warning", currentQuery));
                }
            }
            else
            {
                currentQuery = callTools(mMcpDispatcher, toolCalls, maxErrors, numErrors);
                allowTools = allowTools && numErrors < maxErrors - 1;
                if(numErrors >= maxErrors)
                {
                    // The tool call failed for too many consecutive iterations,
                    // we abort the entire query.
                    return juce::Result::fail(juce::translate("Tool call failed: ERROR").replace("warning", currentQuery));
                }
                else if(currentQuery.empty())
                {
                    // The tool call succeeded and there are no tool responses,
                    // we use the last assistant message as the final answer.
                    return juce::Result::ok();
                }
            }
        }
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted."));
    }

    // Max iterations reached - return the last response
    return juce::Result::fail(juce::translate("Maximum number of iterations reached."));
}

juce::Result Application::Neuralyzer::AgentLocal::sendQuery(juce::String const& prompt)
{
    {
        std::unique_lock<std::mutex> lock(mTemporaryMutex);
        mTempResponse.clear();
    }

    auto const prompGuardResult = mGuard.checkText(prompt);
    if(prompGuardResult.failed())
    {
        common_chat_msg userMsg;
        userMsg.role = "warning";
        userMsg.content = prompGuardResult.getErrorMessage().toStdString();
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages.push_back(std::move(userMsg));
        notifyStateChanged();
        return juce::Result::ok();
    }

    juce::String fullQuery;
    // Adds the RAG context to the query
    // With normalized embeddings + inner-product space, lower distance means better match.
    auto const resources = mRag.computeResources(prompt);
    if(!resources.empty())
    {
        fullQuery << "Relevant documentation excerpts:\n";
        for(auto const& resource : resources)
        {
            fullQuery << "\n";
            fullQuery << "[" << resource.document << " | " << resource.section << "]\n";
            // Remove all line breaks, tabs, etc. from the excerpt to avoid formatting issues in the model's response
            fullQuery << resource.content.replaceCharacters("\n\r\t", "   ") << "\n";
        }
        fullQuery << "\n";
    }

    // Adds the user request to the query
    fullQuery << "User request:\n" + prompt;
    return performQuery(fullQuery, true);
}

juce::Result Application::Neuralyzer::AgentLocal::startSession()
{
    auto resetResult = clearContextMemory();
    return resetResult.wasOk() ? performQuery(getFirstQuery(), false) : resetResult;
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

    std::vector<common_chat_msg> restoredMessages;
    try
    {
        auto const root = nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format: FLNAME").replace("FLNAME", sessionFile.getFullPathName()));
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
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Failed to parse message state file: ") + sessionFile.getFullPathName() + juce::String(" error: ") + exception.what());
        return juce::Result::fail(juce::translate("Failed to parse message state file: FLNAME").replace("FLNAME", sessionFile.getFullPathName()));
    }

    if(restoredMessages.size() <= kNumInitMessages)
    {
        return juce::Result::fail(juce::translate("No messages found in session file."));
    }

    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        mChatInputs.messages = std::move(restoredMessages);
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    size_t nloaded = 0;
    auto* context = mInitResult->context();
    std::vector<llama_token> restoredTokens(static_cast<size_t>(llama_n_ctx(context)));
    auto const contextFile = sessionFile.withFileExtension(".ctx");
    if(contextFile.getSize() <= 0 || !llama_state_load_file(context, contextFile.getFullPathName().toRawUTF8(), restoredTokens.data(), restoredTokens.size(), &nloaded) || nloaded == 0)
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
            MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Failed to apply chat templates when restoring session: ") + e.what());
            return juce::Result::fail(juce::translate("Failed to apply chat templates: ISSUE.").replace("ISSUE", e.what()));
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
                MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to decode message history into context.");
                return juce::Result::fail(juce::translate("Failed to decode message history into context."));
            }
            position += size;
            updateContextMemoryUsage();
        }
        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentLocal", juce::String("Restored ") + juce::String(tokens.size()) + juce::String(" tokens from message history: ") + sessionFile.getFullPathName());
        return juce::Result::ok();
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

    // Set log callback to suppress unnecessary output
    llama_log_set(logCallback, nullptr);

    std::vector<common_chat_msg> messages;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        messages = mChatInputs.messages;
    }
    if(messages.size() <= kNumInitMessages)
    {
        return juce::Result::fail(juce::translate("No messages to save in session."));
    }

    auto* context = mInitResult->context();
    common_chat_params stateParams;
    common_chat_templates_inputs stateInputs;
    {
        std::unique_lock<std::mutex> lock(mHistoryMutex);
        stateInputs = mChatInputs;
    }
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

    nlohmann::json root;
    root["version"] = 1;
    root["messages"] = nlohmann::json::array();
    for(auto const& message : messages)
    {
        nlohmann::json messageJson;
        to_json(messageJson, message);
        root["messages"].push_back(std::move(messageJson));
    }

    if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
    {
        MiscDebug("Application::Neuralyzer::AgentLocal", "Failed to save state metadata file: " + sessionFile.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state metadata file: FLNAME").replace("FLNAME", sessionFile.getFullPathName()));
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
    notifyStateChanged();
    auto* context = mInitResult->context();
    llama_memory_seq_rm(llama_get_memory(context), 0, -1, -1);
    updateContextMemoryUsage();
    return juce::Result::ok();
}

Application::Neuralyzer::AgentLocal::History Application::Neuralyzer::AgentLocal::getHistory() const
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
