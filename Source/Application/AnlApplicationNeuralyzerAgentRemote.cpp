#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

static juce::String sendRequest(juce::URL const& url, std::atomic<bool> const& shouldQuit)
{
    int statusCode = 0;
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                             .withExtraHeaders("Content-Type: application/json")
                             .withStatusCode(&statusCode)
                             .withProgressCallback([&](int, int)
                                                   {
                                                       return !shouldQuit.load();
                                                   });

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
        mLastResponseId.clear();
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
        modelResponse = sendRequest(modelListUrl, mShouldQuit);
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
        mLastResponseId.clear();
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

    juce::String previousResponseId;
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.push_back({role, prompt});
        previousResponseId = mLastResponseId;
    }

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    auto const openAiUrl = info.modelUrl.withNewSubPath("/v1/responses");
    if(!openAiUrl.isWellFormed())
    {
        return createError(juce::translate("Invalid chat URL: ") + openAiUrl.toString(true));
    }

    nlohmann::json request;
    request["model"] = info.modelId;
    auto& input = request["input"];
    auto const addMessage = [&](auto const& type, auto const& content)
    {
        nlohmann::json msg;
        switch(type)
        {
            case MessageType::system:
            {
                break;
            }
            case MessageType::user:
            case MessageType::tool:
            {
                input.push_back({{"role", "user"}, {"content", content}});
                break;
            }
            case MessageType::assistant:
            {
                input.push_back({{"role", "assistant"}, {"content", content}});
                break;
            }
        }
    };
    if(previousResponseId.isEmpty())
    {
        request["instructions"] = mMcpDispatcher.getInstructions();
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        for(auto const& [type, content] : mHistory)
        {
            addMessage(type, content);
        }
    }
    else
    {
        request["previous_response_id"] = previousResponseId;
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        addMessage(mHistory.back().first, mHistory.back().second);
    }
    if(allowTools)
    {
#if 1 // PARTIELS_USE_EPHEMERAL_MCP
        auto& integrations = request["integrations"];
        nlohmann::json server;
        server["type"] = "ephemeral_mcp";
        server["server_label"] = "Partiels";
        server["server_url"] = "http://" + juce::IPAddress::getLocalAddress().toString() + ":" + juce::String(Mcp::Host::defaultPort) + "/mcp";
        integrations.push_back(std::move(server));
#else
        auto& tools = request["tools"];
        for(auto const& tool : mMcpDispatcher.getToolList())
        {
            nlohmann::json item;
            item["type"] = "function";
            item["name"] = tool.name;
            item["description"] = tool.description;
            item["parameters"] = !tool.parameters.empty() ? nlohmann::json::parse(tool.parameters) : nlohmann::json::object();
            tools.push_back(std::move(item));
        }
#endif
    }

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

    juce::String result;
    try
    {
        result = sendRequest(openAiUrl.withPOSTData(juce::String(request.dump())), mShouldQuit);
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

    MiscDebug("Application::Neuralyzer::AgentRemote", "Received response from remote server: " + juce::String(response.dump()).replace("\n", ""));

    MiscWeakAssert(response.contains("id") && response.at("id").is_string());
    if(response.contains("id") && response.at("id").is_string())
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mLastResponseId = juce::String(response.at("id").get<std::string>());
    }

    MiscWeakAssert(response.contains("output") && response.at("output").is_array() && !response.at("output").empty());
    if(!response.contains("output") || !response.at("output").is_array() || response.at("output").empty())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Invalid response format from remote server: " + response.dump());
        return createError(juce::translate("Invalid response format from remote server: CONTENT").replace("CONTENT", juce::String(response.dump())));
    }

    auto [assistantOutput, assitantToolCalls] = [&]()
    {
        std::string aOutput;
        std::vector<common_chat_tool_call> aToolCalls;
        for(auto const& outputItem : response.at("output"))
        {
            MiscWeakAssert(outputItem.contains("type") && outputItem.at("type").is_string());
            auto const outputType = outputItem.value("type", std::string{});
            if(outputType == "message")
            {
                MiscWeakAssert(outputItem.contains("content") && outputItem.at("content").is_array() && !outputItem.at("content").empty());
                if(outputItem.contains("content") && outputItem.at("content").is_array())
                {
                    for(auto const& content : outputItem.at("content"))
                    {
                        MiscWeakAssert(content.contains("type") && content.at("type").is_string());
                        auto const contentType = content.value("type", std::string{});
                        if(contentType == "output_text" || contentType == "text")
                        {
                            aOutput += content.value("text", std::string{});
                        }
                    }
                }
            }
            else if(outputType == "function_call")
            {
                common_chat_tool_call toolCall;
                toolCall.name = outputItem.value("name", std::string{});
                toolCall.arguments = outputItem.value("arguments", std::string{});
                toolCall.id = outputItem.value("call_id", outputItem.value("id", std::string{}));
                MiscWeakAssert(!toolCall.name.empty());
                if(!toolCall.name.empty())
                {
                    aToolCalls.push_back(std::move(toolCall));
                }
            }
        }
        return std::make_tuple(aOutput, aToolCalls);
    }();

    if(assistantOutput.empty() && assitantToolCalls.empty())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "No chat messages found in response");
        return createError(juce::translate("No chat messages found in response"));
    }

    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mTempResponse = juce::String(assistantOutput);
        mHistory.push_back({MessageType::assistant, juce::String(assistantOutput)});
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

    return std::make_tuple(juce::Result::ok(), std::move(assitantToolCalls));
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
        mLastResponseId.clear();
        mHistory.push_back({MessageType::system, mMcpDispatcher.getInstructions()});
    }
    return performQuery(getFirstQuery(), true);
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
