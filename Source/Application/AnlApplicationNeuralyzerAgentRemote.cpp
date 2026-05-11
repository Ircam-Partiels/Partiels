#include "AnlApplicationNeuralyzerAgentRemote.h"

ANALYSE_FILE_BEGIN

static std::tuple<juce::Result, juce::String> sendPostRequest(juce::URL const& url, juce::String const& jsonBody)
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

static std::tuple<juce::Result, juce::String> sendGetRequest(juce::URL const& url)
{
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withExtraHeaders("Content-Type: application/json")
                       .withStatusCode(&statusCode)
                       .withConnectionTimeoutMs(30000);

    auto stream = url.createInputStream(options);
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

    // Fetch model information from remote server (OpenAI API)
    nlohmann::json response;
    auto const modelListUrl = copy.modelUrl.withNewSubPath("/v1/models");
    if(!modelListUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid model list URL: ") + modelListUrl.toString(true));
    }

    auto const modelResult = sendGetRequest(modelListUrl);
    if(std::get<0>(modelResult).failed())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve model info: " + std::get<0>(modelResult).getErrorMessage().toStdString());
        return std::get<0>(modelResult);
    }

    try
    {
        response = nlohmann::json::parse(std::get<1>(modelResult).toStdString());
        if(!response.contains("data") || !response.at("data").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid model list response format"));
        }
    }
    catch(...)
    {
        return juce::Result::fail(juce::translate("Failed to parse model list response"));
    }

    if(copy.modelId.isEmpty())
    {
        for(auto const& modelJson : response.at("data"))
        {
            if(modelJson.contains("id") && modelJson.at("id").is_string())
            {
                copy.modelId = juce::String(modelJson.at("id").get<std::string>());
                break;
            }
        }
    }

    auto const modelId = copy.modelId;
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
            assignOptionalFloatIfMissing(copy.presencePenalty, "presence_penalty");
        }
    }

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

    if(!info.modelUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid chat URL base: ") + info.modelUrl.toString(true));
    }

    nlohmann::json messages = nlohmann::json::array();
    {
        std::unique_lock<std::mutex> sessionLock(mSessionMutex);

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

        nlohmann::json userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = prompt.toStdString();
        messages.push_back(std::move(userMsg));

        // Add user message to history
        mHistory.push_back({MessageType::user, prompt});
    }

    // OpenAI Chat Completions endpoint
    auto const openAiUrl = info.modelUrl.withNewSubPath("/v1/chat/completions");
    if(!openAiUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid chat URL: ") + openAiUrl.toString(true));
    }

    nlohmann::json request;
    request["model"] = info.modelId.toStdString();
    request["messages"] = messages;
    request["stream"] = false;
    request["max_tokens"] = 8000;
    if(info.temperature.has_value())
    {
        request["temperature"] = info.temperature.value();
    }
    if(info.topP.has_value())
    {
        request["top_p"] = info.topP.value();
    }
    if(info.presencePenalty.has_value())
    {
        request["presence_penalty"] = info.presencePenalty.value();
    }

    auto const result = sendPostRequest(openAiUrl, juce::String(request.dump()));

    if(std::get<0>(result).failed())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Send request failed on OpenAI endpoint: " + std::get<0>(result).getErrorMessage().toStdString());
        return std::get<0>(result);
    }

    // Parse response
    try
    {
        auto const response = nlohmann::json::parse(std::get<1>(result).toStdString());

        std::string assistantResponse;

        if(response.contains("choices") && response.at("choices").is_array() && !response.at("choices").empty())
        {
            auto const& choice = response.at("choices").at(0);
            if(choice.contains("message") && choice.at("message").is_object())
            {
                auto const& message = choice.at("message");
                if(message.contains("content") && message.at("content").is_string())
                {
                    assistantResponse += message.at("content").get<std::string>();
                }
                else if(message.contains("content") && message.at("content").is_array())
                {
                    for(auto const& part : message.at("content"))
                    {
                        if(part.is_string())
                        {
                            assistantResponse += part.get<std::string>();
                        }
                        else if(part.is_object() && part.contains("text") && part.at("text").is_string())
                        {
                            assistantResponse += part.at("text").get<std::string>();
                        }
                    }
                }
            }
            else if(choice.contains("text") && choice.at("text").is_string())
            {
                assistantResponse += choice.at("text").get<std::string>();
            }
        }
        else
        {
            return juce::Result::fail(juce::translate("Invalid response format from remote server"));
        }

        {
            std::unique_lock<std::mutex> sessionLock(mSessionMutex);
            mHistory.push_back({MessageType::assistant, juce::String(assistantResponse)});
            mTempResponse = assistantResponse;

            mLastResponseId.clear();

            int inputTokens = 0;
            int outputTokens = 0;
            if(response.contains("usage") && response.at("usage").is_object())
            {
                inputTokens = response.at("usage").value("prompt_tokens", 0);
                outputTokens = response.at("usage").value("completion_tokens", 0);
            }

            if(info.contextSize.has_value() && info.contextSize.value() > 0)
            {
                auto const contextSize = static_cast<float>(info.contextSize.value());
                mContextCapacityUsage.store(static_cast<float>(inputTokens + outputTokens) / contextSize);
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
