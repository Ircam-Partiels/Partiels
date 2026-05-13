#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

static juce::String sendRequest(juce::URL const& url)
{
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withExtraHeaders("Content-Type: application/json")
                       .withStatusCode(&statusCode)
                       .withConnectionTimeoutMs(30000);

    auto stream = url.createInputStream(options);
    if(stream == nullptr)
    {
        throw std::runtime_error(juce::translate("Failed to connect to remote server at URL").replace("URL", url.toString(true)).toStdString());
    }
    auto const responseBody = stream->readEntireStreamAsString();
    if(statusCode < 200 || statusCode >= 300)
    {
        throw std::runtime_error(juce::translate("Remote server URL returned HTTP CODE: MSG").replace("URL", url.toString(true)).replace("CODE", juce::String(statusCode)).replace("MSG", responseBody).toStdString());
    }
    return responseBody;
}

Application::Neuralyzer::AgentRemote::AgentRemote(Mcp::Dispatcher& mcpDispatcher)
: mMcpDispatcher(mcpDispatcher)
, mContextCapacityUsage(0.0f)
{
}

void Application::Neuralyzer::AgentRemote::setFirstQuery(juce::String const& firstQuery)
{
    std::unique_lock<std::mutex> lock(mConfigMutex);
    mFirstQuery = firstQuery;
}

juce::String Application::Neuralyzer::AgentRemote::getFirstQuery() const
{
    std::unique_lock<std::mutex> lock(mConfigMutex);
    return mFirstQuery;
}

juce::Result Application::Neuralyzer::AgentRemote::initializeModel(ModelInfo const& info)
{
    auto copy = info;
    if(!copy.modelUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid model URL: ") + copy.modelUrl.toString(true));
    }

    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mContextCapacityUsage.store(0.0f);
    }

    nlohmann::json response;
    auto const modelListUrl = copy.modelUrl.withNewSubPath("/v1/models");
    if(!modelListUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid model list URL: ") + modelListUrl.toString(true));
    }

    juce::String modelResponse;
    try
    {
        modelResponse = sendRequest(modelListUrl);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve model info: " + juce::String(e.what()));
        return juce::Result::fail(juce::String(e.what()));
    }

    try
    {
        response = nlohmann::json::parse(modelResponse.toStdString());
        if(!response.contains("data") || !response.at("data").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid model list response format"));
        }
    }
    catch(...)
    {
        return juce::Result::fail(juce::translate("Failed to parse model list response"));
    }

    auto const modelId = copy.modelId;
    auto const modelIt = std::find_if(response.at("data").cbegin(), response.at("data").cend(), [&modelId](auto const& modelJson)
                                      {
                                          return modelJson.value("id", std::string{}) == modelId.toStdString();
                                      });
    if(modelIt == response.at("data").cend())
    {
        return juce::Result::fail(juce::translate("Model MODELID not found in model list response.").replace("MODELID", modelId));
    }

    {
        auto const assignOptionalFloatIfMissing = [&](std::optional<float>& target, std::vector<std::string> const& keys)
        {
            if(target.has_value())
            {
                return;
            }

            auto const* params = (modelIt->contains("parameters") && modelIt->at("parameters").is_object()) ? &modelIt->at("parameters") : nullptr;

            for(auto const& key : keys)
            {
                if(modelIt->contains(key) && modelIt->at(key).is_number())
                {
                    target = static_cast<float>(modelIt->at(key).get<double>());
                    return;
                }
                if(params != nullptr && params->contains(key) && params->at(key).is_number())
                {
                    target = static_cast<float>(params->at(key).get<double>());
                    return;
                }
            }
        };

        auto const assignOptionalIntIfMissing = [&](std::optional<int32_t>& target, std::vector<std::string> const& keys)
        {
            if(target.has_value())
            {
                return;
            }

            auto const* params = (modelIt->contains("parameters") && modelIt->at("parameters").is_object()) ? &modelIt->at("parameters") : nullptr;

            auto const readInt = [](nlohmann::json const& node) -> std::optional<int32_t>
            {
                if(node.is_number_integer())
                {
                    auto const value = node.get<int64_t>();
                    if(value >= static_cast<int64_t>(std::numeric_limits<int32_t>::min()) && value <= static_cast<int64_t>(std::numeric_limits<int32_t>::max()))
                    {
                        return static_cast<int32_t>(value);
                    }
                }
                if(node.is_number_unsigned())
                {
                    auto const value = node.get<uint64_t>();
                    if(value <= static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
                    {
                        return static_cast<int32_t>(value);
                    }
                }
                return std::nullopt;
            };

            for(auto const& key : keys)
            {
                if(modelIt->contains(key))
                {
                    auto const value = readInt(modelIt->at(key));
                    if(value.has_value())
                    {
                        target = value.value();
                        return;
                    }
                }

                if(params != nullptr && params->contains(key))
                {
                    auto const value = readInt(params->at(key));
                    if(value.has_value())
                    {
                        target = value.value();
                        return;
                    }
                }
            }
        };

        assignOptionalIntIfMissing(copy.contextSize, {"context_window", "n_ctx", "max_context_length", "max_sequence_length"});
        assignOptionalIntIfMissing(copy.batchSize, {"batch_size", "n_batch"});
        assignOptionalFloatIfMissing(copy.minP, {"min_p"});
        assignOptionalFloatIfMissing(copy.temperature, {"temperature"});
        assignOptionalFloatIfMissing(copy.topP, {"top_p"});
        assignOptionalIntIfMissing(copy.topK, {"top_k"});
        assignOptionalFloatIfMissing(copy.presencePenalty, {"presence_penalty"});
        assignOptionalFloatIfMissing(copy.repetitionPenalty, {"repeat_penalty", "repetition_penalty"});

        std::unique_lock<std::mutex> configLock(mConfigMutex);
        mModelInfo = copy;
    }

    MiscDebug("Application::Neuralyzer::AgentRemote", "Initialized with model: " + modelId + " at " + modelListUrl.toString(true));
    return juce::Result::ok();
}

juce::Result Application::Neuralyzer::AgentRemote::resetModel()
{
    {
        std::unique_lock<std::mutex> configLock(mConfigMutex);
        mModelInfo = ModelInfo{};
    }
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mContextCapacityUsage.store(0.0f);
    }
    return juce::Result::ok();
}

std::tuple<juce::Result, std::vector<common_chat_tool_call>> Application::Neuralyzer::AgentRemote::performMessage(MessageType const& role, juce::String const& prompt, bool allowTools)
{
    auto const info = [&]()
    {
        std::unique_lock<std::mutex> configLock(mConfigMutex);
        return mModelInfo;
    }();

    auto const createError = [](juce::String error)
    {
        return std::make_tuple(juce::Result::fail(std::move(error)), std::vector<common_chat_tool_call>{});
    };

    if(info.modelUrl.isEmpty() || info.modelId.isEmpty())
    {
        return createError(juce::translate("Remote server model not initialized"));
    }

    if(!info.modelUrl.isWellFormed())
    {
        return createError(juce::translate("Invalid chat URL base: ") + info.modelUrl.toString(true));
    }

    nlohmann::json messages = nlohmann::json::array();
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        // Add user message to history
        mHistory.push_back({role, prompt});
        for(auto const& [type, content] : mHistory)
        {
            nlohmann::json msg;
            switch(type)
            {
                case MessageType::system:
                {
                    msg["role"] = "system";
                    break;
                }
                case MessageType::user:
                {
                    msg["role"] = "user";
                    break;
                }
                case MessageType::assistant:
                {
                    msg["role"] = "assistant";
                    break;
                }
                case MessageType::tool:
                {
                    msg["role"] = "tool";
                    break;
                }
            }

            msg["content"] = content.toStdString();
            messages.push_back(std::move(msg));
        }
    }

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    // OpenAI Chat Completions endpoint
    auto const openAiUrl = info.modelUrl.withNewSubPath("/v1/chat/completions");
    if(!openAiUrl.isWellFormed())
    {
        return createError(juce::translate("Invalid chat URL: ") + openAiUrl.toString(true));
    }

    nlohmann::json request;
    request["model"] = info.modelId.toStdString();
    request["messages"] = messages;
    request["stream"] = false;
    request["max_tokens"] = 8000;
    if(info.contextSize.has_value())
    {
        request["context_size"] = info.contextSize.value();
        request["context_window"] = info.contextSize.value();
        request["n_ctx"] = info.contextSize.value();
    }
    if(info.batchSize.has_value())
    {
        request["batch_size"] = info.batchSize.value();
        request["n_batch"] = info.batchSize.value();
    }
    if(info.minP.has_value())
    {
        request["min_p"] = info.minP.value();
    }
    if(info.temperature.has_value())
    {
        request["temperature"] = info.temperature.value();
    }
    if(info.topP.has_value())
    {
        request["top_p"] = info.topP.value();
    }
    if(info.topK.has_value())
    {
        request["top_k"] = info.topK.value();
    }
    if(info.presencePenalty.has_value())
    {
        request["presence_penalty"] = info.presencePenalty.value();
    }
    if(info.repetitionPenalty.has_value())
    {
        request["repeat_penalty"] = info.repetitionPenalty.value();
        request["repetition_penalty"] = info.repetitionPenalty.value();
    }

    if(allowTools)
    {
        request["tools"] = common_chat_tools_to_json_oaicompat(mMcpDispatcher.getToolList());
    }

    juce::String result;
    try
    {
        result = sendRequest(openAiUrl.withPOSTData(juce::String(request.dump())));
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: " + juce::String(e.what()));
        return createError(e.what());
    }
    catch(...)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: unknown error");
        return createError(juce::translate("Send request failed on OpenAI endpoint with unknown error"));
    }

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    // Parse response
    nlohmann::json response;
    try
    {
        response = nlohmann::json::parse(result.toStdString());
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: " + juce::String(e.what()));
        return createError(juce::translate("Failed to parse remote server response: ERROR").replace("ERROR", juce::String(e.what())));
    }

    MiscDebug("Application::Neuralyzer::AgentRemote", "Received response from remote server: " + juce::String(response.dump()));

    MiscWeakAssert(response.contains("choices") && response.at("choices").is_array() && !response.at("choices").empty());
    if(!response.contains("choices") || !response.at("choices").is_array() || response.at("choices").empty())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Invalid response format from remote server: " + response.dump());
        return createError(juce::translate("Invalid response format from remote server: CONTENT").replace("CONTENT", juce::String(response.dump())));
    }

    auto choices = nlohmann::json::array();
    for(auto const& choice : response.at("choices"))
    {
        choices.push_back(choice.at("message"));
    }

    std::vector<common_chat_msg> chatMessages;
    try
    {
        chatMessages = common_chat_msgs_parse_oaicompat(choices);
    }
    catch(std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to parse chat messages from response: " + juce::String(e.what()));
        return createError(juce::translate("Failed to parse chat messages from response: ERROR").replace("ERROR", juce::String(e.what())));
    }
    catch(...)
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to parse chat messages from response: unknown error");
        return createError(juce::translate("Failed to parse chat messages from response with unknown error"));
    }

    if(chatMessages.empty() || (chatMessages.at(0).content.empty() && chatMessages.at(0).tool_calls.empty()))
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "No chat messages found in response");
        return createError(juce::translate("No chat messages found in response"));
    }

    auto& chatMessage = chatMessages.at(0);
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        if(chatMessage.content.empty())
        {
            if(!chatMessage.reasoning_content.empty())
            {
                mTempResponse = chatMessage.reasoning_content;
            }
            else
            {
                for(auto const& contentPart : chatMessage.content_parts)
                {
                    mTempResponse += juce::String(contentPart.text);
                }
            }
        }
        else
        {
            mTempResponse = chatMessage.content;
            mHistory.push_back({MessageType::assistant, juce::String(chatMessage.content)});
        }
    }
    notifyStateChanged();

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    int inputTokens = 0;
    int outputTokens = 0;
    if(response.contains("usage") && response.at("usage").is_object())
    {
        auto const& usage = response.at("usage");
        // Support both OpenAI-style (prompt_tokens/completion_tokens) and Anthropic-style (input_tokens/output_tokens)
        if(usage.contains("prompt_tokens"))
        {
            inputTokens = usage.value("prompt_tokens", 0);
        }
        else
        {
            inputTokens = usage.value("input_tokens", 0);
        }
        if(usage.contains("completion_tokens"))
        {
            outputTokens = usage.value("completion_tokens", 0);
        }
        else
        {
            outputTokens = usage.value("output_tokens", 0);
        }
        // Fallback: use total_tokens if individual counts are unavailable
        if(inputTokens == 0 && outputTokens == 0)
        {
            inputTokens = usage.value("total_tokens", 0);
        }
    }

    if(info.contextSize.has_value() && info.contextSize.value() > 0)
    {
        auto const contextSize = static_cast<float>(info.contextSize.value());
        mContextCapacityUsage.store(static_cast<float>(inputTokens + outputTokens) / contextSize);
    }

    return std::make_tuple(juce::Result::ok(), std::move(chatMessage.tool_calls));
}

juce::Result Application::Neuralyzer::AgentRemote::performQuery(juce::String const& prompt, bool allowTools)
{
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mTempResponse.clear();
    }
    MiscDebug("Application::Neuralyzer::AgentRemote", "Query completed successfully");

    auto currentQuery = prompt;
    auto numErrors = 0_z;
    static auto constexpr maxErrors = 3_z;      // Abort if too perform errors
    static auto constexpr maxIterations = 25_z; // Prevent infinite loops
    MessageType role = MessageType::user;
    for(auto iteration = 0_z; iteration < maxIterations && mShouldQuit.load() == false; ++iteration)
    {
        auto const [result, toolCalls] = performMessage(role, currentQuery, allowTools);
        if(result.failed())
        {
            return result;
        }
        else
        {
            role = MessageType::tool;
            currentQuery = callTools(mMcpDispatcher, toolCalls, maxErrors, numErrors);
            allowTools = allowTools && numErrors < maxErrors - 1;
            if(numErrors >= maxErrors)
            {
                // The tool call failed for too many consecutive iterations,
                // we abort the entire query.
                return juce::Result::fail(juce::translate("Tool call failed: ERROR").replace("ERROR", currentQuery));
            }
            else if(currentQuery.isEmpty())
            {
                // The tool call succeeded and there are no tool responses,
                // we use the last assistant message as the final answer.
                return juce::Result::ok();
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

juce::Result Application::Neuralyzer::AgentRemote::sendQuery(juce::String const& prompt)
{
    return performQuery(prompt, true);
}

juce::Result Application::Neuralyzer::AgentRemote::startSession()
{
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mHistory.push_back({MessageType::system, mMcpDispatcher.getInstructions()});
    }
    return performQuery(getFirstQuery(), true);
}

juce::Result Application::Neuralyzer::AgentRemote::loadSession(juce::File const& sessionFile)
{
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file configured"));
    }

    if(sessionFile.getSize() <= 0)
    {
        return juce::Result::fail(juce::translate("The session file is empty"));
    }

    try
    {
        auto const root = nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format"));
        }

        History newHistory;
        for(auto const& msgJson : root.at("messages"))
        {
            auto const roleStr = msgJson.value("role", std::string{});
            auto const content = msgJson.value("content", std::string{});

            MessageType type = MessageType::user;
            if(roleStr == "system")
                type = MessageType::system;
            else if(roleStr == "user")
                type = MessageType::user;
            else if(roleStr == "assistant")
                type = MessageType::assistant;
            else if(roleStr == "tool")
                type = MessageType::tool;

            newHistory.push_back({type, juce::String(content)});
        }

        {
            std::unique_lock<std::mutex> sessionLock(mSessionMutex);
            mHistory = std::move(newHistory);
        }

        notifyStateChanged();
        MiscDebug("Application::Neuralyzer::AgentRemote", "Session loaded with " + juce::String(mHistory.size()) + " messages");
        return juce::Result::ok();
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to load session: ") + juce::String(e.what()));
    }
}

juce::Result Application::Neuralyzer::AgentRemote::saveSession(juce::File const& sessionFile)
{
    if(sessionFile == juce::File{})
    {
        return juce::Result::fail(juce::translate("No session file configured"));
    }

    // Ensure directory exists
    if(!sessionFile.getParentDirectory().createDirectory())
    {
        return juce::Result::fail(juce::translate("Failed to create session directory"));
    }

    History historyCopy;
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        historyCopy = mHistory;
    }
    if(historyCopy.empty())
    {
        return juce::Result::fail(juce::translate("No messages to save in session"));
    }

    try
    {
        nlohmann::json root;
        root["version"] = 1;
        root["messages"] = nlohmann::json::array();

        for(auto const& [type, content] : historyCopy)
        {
            nlohmann::json msg;
            switch(type)
            {
                case MessageType::system:
                {
                    msg["role"] = "system";
                    break;
                }
                case MessageType::user:
                {
                    msg["role"] = "user";
                    break;
                }
                case MessageType::assistant:
                {
                    msg["role"] = "assistant";
                    break;
                }
                case MessageType::tool:
                {
                    msg["role"] = "tool";
                    break;
                }
            }

            msg["content"] = content.toStdString();
            root["messages"].push_back(std::move(msg));
        }

        if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
        {
            return juce::Result::fail(juce::translate("Failed to write session file"));
        }

        MiscDebug("Application::Neuralyzer::AgentRemote", "Session saved with " + juce::String(mHistory.size()) + " messages");
        return juce::Result::ok();
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to save session: ") + juce::String(e.what()));
    }
}

Application::Neuralyzer::Agent::History Application::Neuralyzer::AgentRemote::getHistory() const
{
    std::unique_lock<std::mutex> sessionLock(mSessionMutex);
    return mHistory;
}

juce::String Application::Neuralyzer::AgentRemote::getTemporaryResponse() const
{
    std::unique_lock<std::mutex> sessionLock(mSessionMutex);
    return mTempResponse;
}

float Application::Neuralyzer::AgentRemote::getContextCapacityUsage() const
{
    return mContextCapacityUsage.load();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::AgentRemote::getModelInfo() const
{
    std::unique_lock<std::mutex> configLock(mConfigMutex);
    return mModelInfo;
}

void Application::Neuralyzer::AgentRemote::setShouldQuit(bool state)
{
    mShouldQuit.store(state);
}

bool Application::Neuralyzer::AgentRemote::shouldQuit() const
{
    return mShouldQuit.load();
}

void Application::Neuralyzer::AgentRemote::setNotifyCallback(std::function<void()> callback)
{
    std::unique_lock<std::mutex> callbackLock(mCallbackMutex);
    mNotifyCallback = std::move(callback);
}

void Application::Neuralyzer::AgentRemote::notifyStateChanged()
{
    std::function<void()> callback;
    {
        std::unique_lock<std::mutex> callbackLock(mCallbackMutex);
        callback = mNotifyCallback;
    }

    if(callback)
    {
        callback();
    }
}

ANALYSE_FILE_END
