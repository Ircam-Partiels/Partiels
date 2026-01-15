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

static std::optional<int32_t> getModelContextSize(juce::URL const& modelUrl, juce::String const& modelId, std::atomic<bool> const& shouldQuit)
{
    // llama.cpp servers expose the context size in the /props endpoint
    auto const propsUrl = modelUrl.withNewSubPath("/props");
    if(propsUrl.isWellFormed())
    {
        try
        {
            auto const propsResponse = sendRequest(propsUrl, shouldQuit);
            auto propsJson = nlohmann::json::parse(propsResponse.toStdString());
            auto const contextSize = propsJson["default_generation_settings"].value("n_ctx", 0);
            if(contextSize > 0)
            {
                return contextSize;
            }
        }
        catch(std::exception const& e)
        {
            MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve context size from /props: " + juce::String(e.what()));
        }
    }

    // LM Studio servers expose the context size in the /api/v1/models endpoint
    auto const modelsApiUrl = modelUrl.withNewSubPath("/api/v1/models");
    if(modelsApiUrl.isWellFormed())
    {
        try
        {
            auto const modelsResponse = sendRequest(modelsApiUrl, shouldQuit);
            auto reponseJson = nlohmann::json::parse(modelsResponse.toStdString());
            if(reponseJson.contains("models") && reponseJson.at("models").is_array())
            {
                auto& modelsJson = reponseJson["models"];
                auto modelIt = std::find_if(modelsJson.begin(), modelsJson.end(), [&](auto const& model)
                                            {
                                                return model.is_object() && model.value("key", juce::String{}) == modelId;
                                            });
                if(modelIt != modelsJson.end())
                {
                    for(auto& instance : (*modelIt)["loaded_instances"])
                    {
                        auto const contextSize = instance["config"].value("context_length", 0);
                        if(contextSize > 0)
                        {
                            return contextSize;
                        }
                    }
                }
            }
        }
        catch(std::exception const& e)
        {
            MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve context size from /api/v1/models: " + juce::String(e.what()));
        }
    }
    return std::nullopt;
}

std::set<juce::String> Application::Neuralyzer::AgentRemote::getAvailableModels(juce::URL const& serverUrl)
{
    std::set<juce::String> models;
    if(serverUrl.isEmpty())
    {
        return models;
    }

    int statusCode = 0;
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                             .withExtraHeaders("Content-Type: application/json")
                             .withStatusCode(&statusCode)
                             .withConnectionTimeoutMs(30000);
    auto const stream = serverUrl.withNewSubPath("/v1/models").createInputStream(options);
    if(stream == nullptr || statusCode < 200 || statusCode >= 300)
    {
        MiscWeakAssert(false);
        return models;
    }

    auto const responseBody = stream->readEntireStreamAsString();
    MiscWeakAssert(responseBody.isNotEmpty());
    if(responseBody.isEmpty())
    {
        return models;
    }

    auto const response = [&]() -> nlohmann::json
    {
        try
        {
            return nlohmann::json::parse(responseBody.toStdString());
        }
        catch(...)
        {
            return {};
        }
    }();

    MiscWeakAssert(response.contains("data") && response.at("data").is_array());
    if(response.contains("data") && response.at("data").is_array())
    {
        for(auto const& model : response.at("data"))
        {
            MiscWeakAssert(model.contains("id") && model.at("id").is_string());
            if(model.contains("id") && model.at("id").is_string())
            {
                models.insert(model.at("id").get<juce::String>());
            }
        }
    }
    return models;
}

Application::Neuralyzer::AgentRemote::AgentRemote(Mcp::Dispatcher& mcpDispatcher, Rag::Engine& ragEngine)
: mMcpDispatcher(mcpDispatcher)
, mRagEngine(ragEngine)
{
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
                                          return modelJson.value("id", juce::String{}) == modelId;
                                      });
    if(modelIt == response.at("data").cend())
    {
        return juce::Result::fail(juce::translate("Model MODELID not found in model list response.").replace("MODELID", modelId));
    }

    copy.contextSize = getModelContextSize(copy.modelUrl, copy.modelId, mShouldQuit);

    {
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
    auto const info = getModelInfo();

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

    auto const history = [&]()
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.push_back({role, prompt});
        return mHistory;
    }();

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    auto const openAiUrl = info.modelUrl.withNewSubPath("/v1/chat/completions");
    if(!openAiUrl.isWellFormed())
    {
        return createError(juce::translate("Invalid chat URL: ") + openAiUrl.toString(true));
    }

    nlohmann::json request;
    request["model"] = info.modelId;
    auto& input = request["messages"];
    for(auto const& [type, content] : history)
    {
        switch(type)
        {
            case MessageType::system:
            {
                input.push_back({{"role", "system"}, {"content", content}});
                break;
            }
            case MessageType::user:
            {
                input.push_back({{"role", "user"}, {"content", content}});
                break;
            }
            case MessageType::tool:
            {
                // Tool results are fed back as user-visible context for the next turn.
                input.push_back({{"role", "user"}, {"content", content}});
                break;
            }
            case MessageType::assistant:
            {
                input.push_back({{"role", "assistant"}, {"content", content}});
                break;
            }
            case MessageType::warning:
                // Error messages are not sent to the model, they are only used for internal state and debugging.
                break;
        }
    }

    if(allowTools)
    {
        request["tools"] = common_chat_tools_to_json_oaicompat(mMcpDispatcher.getToolList());
    }
    MiscDebug("Application::Neuralyzer::AgentRemote", "Send: " + request.dump());

    juce::String result;
    try
    {
        result = sendRequest(openAiUrl.withPOSTData(juce::String(request.dump())), mShouldQuit);
    }
    catch(std::exception const& e)
    {
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: " + juce::String::fromUTF8(e.what()));
        return createError(e.what());
    }
    catch(...)
    {
        MiscWeakAssert(false);
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
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: " + juce::String(e.what()));
        return createError(juce::translate("Failed to parse remote server response: ERROR").replace("ERROR", juce::String(e.what())));
    }

    MiscDebug("Application::Neuralyzer::AgentRemote", "Received response from remote server: " + juce::String(response.dump()).replace("\n", ""));

    MiscWeakAssert(response.contains("choices") && response.at("choices").is_array() && !response.at("choices").empty());
    if(!response.contains("choices") || !response.at("choices").is_array() || response.at("choices").empty())
    {
        MiscWeakAssert(false);
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
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to parse chat messages from response: " + juce::String::fromUTF8(e.what()));
        return createError(juce::translate("Failed to parse chat messages from response: ERROR").replace("ERROR", juce::String::fromUTF8(e.what())));
    }
    catch(...)
    {
        MiscWeakAssert(false);
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to parse chat messages from response: unknown error");
        return createError(juce::translate("Failed to parse chat messages from response with unknown error"));
    }

    MiscWeakAssert(!chatMessages.empty() && (!chatMessages.at(0).content.empty() || !chatMessages.at(0).tool_calls.empty()));
    if(chatMessages.empty() || (chatMessages.at(0).content.empty() && chatMessages.at(0).tool_calls.empty()))
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "No chat messages found in response");
        return createError(juce::translate("No chat messages found in response"));
    }

    auto& chatMessage = chatMessages.at(0);
    if(chatMessage.content.empty())
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
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
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mTempResponse = chatMessage.content;
        mHistory.push_back({MessageType::assistant, juce::String(chatMessage.content)});
    }

    notifyStateChanged();

    if(mShouldQuit.load())
    {
        return createError(juce::translate("Operation aborted."));
    }

    if(response.contains("usage") && response.at("usage").is_object())
    {
        auto const totalTokens = response.at("usage").value("total_tokens", 0);
        auto const contextSize = getModelInfo().contextSize.value_or(0);
        if(contextSize > 0)
        {
            auto const usage = static_cast<float>(static_cast<double>(totalTokens) / static_cast<double>(contextSize));
            mContextCapacityUsage.store(usage);
        }
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
                notifyStateChanged();
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
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mTempResponse.clear();
    }

    juce::String fullQuery;
    // Adds the RAG context to the query
    // With normalized embeddings + inner-product space, lower distance means better match.
    auto const resources = mRagEngine.computeResources(prompt);
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

juce::Result Application::Neuralyzer::AgentRemote::startSession()
{
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mHistory.push_back({MessageType::system, mMcpDispatcher.getInstructions()});
    }
    return juce::Result::ok();
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
    if(historyCopy.size() <= kNumInitMessages)
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
                case MessageType::warning:
                {
                    msg["role"] = "error";
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

        if(newHistory.size() <= kNumInitMessages)
        {
            return juce::Result::fail(juce::translate("No messages found in session file"));
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
