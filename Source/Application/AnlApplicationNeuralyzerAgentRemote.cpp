#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

static std::tuple<juce::Result, juce::String> sendRequest(juce::URL const& url, juce::String const& jsonBody)
{
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withExtraHeaders("Content-Type: application/json")
                       .withStatusCode(&statusCode)
                       .withConnectionTimeoutMs(30000);

    auto stream = url.withPOSTData(jsonBody).createInputStream(options);
    if(stream == nullptr)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to connect to remote server at URL").replace("URL", url.toString(true))), juce::String{});
    }
    auto const responseBody = stream->readEntireStreamAsString();
    if(statusCode < 200 || statusCode >= 300)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("remote server returned HTTP CODE: MSG").replace("CODE", juce::String(statusCode)).replace("MSG", responseBody.substring(0, 300))), juce::String{});
    }
    return std::make_tuple(juce::Result::ok(), responseBody);
}

Application::Neuralyzer::AgentRemote::AgentRemote()
: mContextCapacityUsage(0.0f)
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
    auto const fullModelUrl = copy.modelUrl.withNewSubPath("/api/v1/models");
    if(!fullModelUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid model URL: ") + copy.modelUrl.toString(true));
    }
    auto const modelId = copy.modelId;

    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mLastResponseId.clear();
        mContextCapacityUsage.store(0.0f);
    }

    // Fetch model information from remote server to get actual context size
    auto const result = sendRequest(fullModelUrl, juce::String(""));
    if(std::get<0>(result).failed())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve model info: " + std::get<0>(result).getErrorMessage().toStdString());
        return std::get<0>(result);
    }
    auto const response = [&]()
    {
        try
        {
            return nlohmann::json::parse(std::get<1>(result).toStdString());
        }
        catch(...)
        {
            return nlohmann::json::object();
        }
    }();
    if(response.contains("data") && response.at("data").is_array())
    {
        auto const modelIt = std::find_if(response.at("data").cbegin(), response.at("data").cend(), [&modelId](auto const& modelJson)
                                          {
                                              return modelJson.value("id", std::string{}) == modelId.toStdString();
                                          });
        if(modelIt != response.at("data").cend())
        {
            auto const assignOptionalFloatIfMissing = [&](std::optional<float>& target, char const* key)
            {
                if(!target.has_value() && modelIt->contains(key) && modelIt->at(key).is_number_float())
                {
                    target = modelIt->at(key).get<float>();
                }
            };

            auto const assignOptionalIntIfMissing = [&](std::optional<int32_t>& target, char const* key)
            {
                if(!target.has_value() && modelIt->contains(key) && modelIt->at(key).is_number_integer())
                {
                    target = modelIt->at(key).get<int32_t>();
                }
            };

            assignOptionalIntIfMissing(copy.contextSize, "context_window");
            assignOptionalFloatIfMissing(copy.minP, "min_p");
            assignOptionalFloatIfMissing(copy.temperature, "temperature");
            assignOptionalFloatIfMissing(copy.topP, "top_p");
            assignOptionalIntIfMissing(copy.topK, "top_k");
            assignOptionalFloatIfMissing(copy.repetitionPenalty, "repeat_penalty");
        }
    }

    {
        std::unique_lock<std::mutex> configLock(mConfigMutex);
        mModelInfo = copy;
    }

    MiscDebug("Application::Neuralyzer::AgentRemote", "Initialized with model: " + modelId + " at " + fullModelUrl.toString(true));
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

juce::Result Application::Neuralyzer::AgentRemote::sendQuery(juce::String const& prompt)
{
    std::unique_lock<std::mutex> configLock(mConfigMutex);
    auto info = mModelInfo;
    configLock.unlock();

    if(info.modelUrl.isEmpty() || info.modelId.isEmpty())
    {
        return juce::Result::fail(juce::translate("Remote server model not initialized"));
    }

    auto const fullChatUrl = info.modelUrl.withNewSubPath("/api/v1/chat");
    if(!fullChatUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid chat URL: ") + fullChatUrl.toString(true));
    }

    // Build request under session lock, then release before HTTP call
    nlohmann::json request;
    request["model"] = info.modelId;
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
    if(info.repetitionPenalty.has_value())
    {
        request["repeat_penalty"] = info.repetitionPenalty.value();
    }
    if(info.contextSize.has_value())
    {
        request["context_length"] = info.contextSize.value();
    }

    request["max_output_tokens"] = 8000;
    request["stream"] = false;
    request["store"] = true; // Enable response storage for stateful chats
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);

        // First message: include full history for context
        if(mLastResponseId.isEmpty())
        {
            // Build full message array only for first message
            nlohmann::json messages = nlohmann::json::array();

            // Add full history for initial context
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

            // Add current user message
            nlohmann::json userMsg;
            userMsg["role"] = "user";
            userMsg["content"] = prompt.toStdString();
            messages.push_back(std::move(userMsg));

            request["input"] = messages;
        }
        else
        {
            // Stateful chat: send only the new user message
            request["input"] = prompt.toStdString();
            request["previous_response_id"] = mLastResponseId.toStdString();
        }

        // Add user message to history
        mHistory.push_back({MessageType::user, prompt});
    }

    // Add Partiels as ephemeral MCP server
    request["integrations"] = nlohmann::json::array();
    nlohmann::json mcpIntegration;
    mcpIntegration["type"] = "ephemeral_mcp";
    mcpIntegration["server_label"] = "Partiels";
    mcpIntegration["server_url"] = "http://localhost:3939";
    request["integrations"].push_back(std::move(mcpIntegration));

    // Send request (no lock held)
    auto const result = sendRequest(fullChatUrl, juce::String(request.dump()));

    if(std::get<0>(result).failed())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed: " + std::get<0>(result).getErrorMessage().toStdString());
        return std::get<0>(result);
    }

    // Parse response
    try
    {
        auto const response = nlohmann::json::parse(std::get<1>(result).toStdString());
        if(!response.contains("output") || !response.at("output").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid response format from remote server"));
        }

        std::string assistantResponse;
        for(auto const& output : response.at("output"))
        {
            if(output.value("type", std::string{}) == "message")
            {
                assistantResponse += output.value("content", std::string{});
            }
            else if(output.value("type", std::string{}) == "tool_call")
            {
                // Tool calls are handled by remote server via Partiels MCP server
                MiscDebug("Application::Neuralyzer::AgentRemote", "Tool call: " + juce::String(output.value("tool", std::string{})));
            }
        }

        {
            std::unique_lock<std::mutex> sessionLock(mSessionMutex);
            mHistory.push_back({MessageType::assistant, juce::String(assistantResponse)});
            mTempResponse = assistantResponse;

            // Store response_id for next stateful chat request
            if(response.contains("response_id"))
            {
                mLastResponseId = juce::String(response.at("response_id").get<std::string>());
            }

            // Update context usage estimate
            if(response.contains("stats") && response.at("stats").contains("input_tokens"))
            {
                auto const inputTokens = response.at("stats").value("input_tokens", 0);
                auto const outputTokens = response.at("stats").value("total_output_tokens", 0);
                if(info.contextSize.has_value() && info.contextSize.value() > 0)
                {
                    auto const contextSize = static_cast<float>(info.contextSize.value());
                    mContextCapacityUsage.store(static_cast<float>(inputTokens + outputTokens) / contextSize);
                }
            }
        }

        notifyStateChanged();

        MiscDebug("Application::Neuralyzer::AgentRemote", "Query completed successfully");
        return juce::Result::ok();
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to parse remote server response: ") + juce::String(e.what()));
    }
}

juce::Result Application::Neuralyzer::AgentRemote::startSession()
{
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);
        mHistory.clear();
        mTempResponse.clear();
        mLastResponseId.clear();
    }
    return sendQuery(getFirstQuery());
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
            mLastResponseId.clear();
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
