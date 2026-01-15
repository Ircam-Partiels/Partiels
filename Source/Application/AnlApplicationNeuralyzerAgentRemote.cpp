#include "AnlApplicationNeuralyzerAgentRemote.h"

static bool isMediaImageFile(juce::File const& file)
{
    return file.hasFileExtension("png;jpg;jpeg;gif;webp");
}

static bool isMediaAudioFile(juce::File const& file)
{
    return file.hasFileExtension("wav;mp3;flac;ogg;m4a");
}

static bool isMediaTextFile(juce::File const& file)
{
    return file.hasFileExtension("txt;md;json;xml;csv;lab;cue");
}

static juce::String getImageMimeType(juce::File const& file)
{
    auto const extension = file.getFileExtension().toLowerCase();
    if(extension == ".jpg" || extension == ".jpeg")
    {
        return "image/jpeg";
    }
    if(extension == ".gif")
    {
        return "image/gif";
    }
    if(extension == ".webp")
    {
        return "image/webp";
    }
    return "image/png";
}

// The OpenAI-compatible "input_audio" content part only documents "wav" and "mp3" as formats.
static juce::String getAudioFormat(juce::File const& file)
{
    auto const extension = file.getFileExtension().toLowerCase();
    return extension.isEmpty() ? juce::String("wav") : extension.substring(1);
}

// Extracts the image_url/input_audio content parts produced by readFiles() into their own content
// parts, since some OpenAI-compatible endpoints require the media to be described as a structured
// content array rather than being flattened into a plain string like the rest of the payload.
static common_chat_msg extractInlineImages(common_chat_msg const& toolMessage)
{
    nlohmann::json parsed;
    try
    {
        parsed = nlohmann::json::parse(toolMessage.content);
    }
    catch(...)
    {
        return toolMessage;
    }

    if(!parsed.contains("content") || !parsed.at("content").is_array())
    {
        return toolMessage;
    }

    auto const hasMedia = std::any_of(parsed.at("content").cbegin(), parsed.at("content").cend(), [](auto const& item)
                                      {
                                          auto const type = item.value("type", std::string{});
                                          return type == "image_url" || type == "input_audio";
                                      });
    if(!hasMedia)
    {
        return toolMessage;
    }

    auto message = toolMessage;
    message.content.clear();
    for(auto const& item : parsed.at("content"))
    {
        auto const type = item.value("type", std::string{});
        common_chat_msg_content_part part;
        if(type == "text")
        {
            part.type = "text";
            part.text = item.value("text", std::string{});
        }
        else if(type == "image_url")
        {
            part.type = "image_url";
            part.text = item.value("image_url", nlohmann::json::object()).value("url", std::string{});
        }
        else if(type == "input_audio")
        {
            // The data and format fields are round-tripped as a JSON blob in .text since
            // common_chat_msg_content_part only carries a single string payload per part.
            part.type = "input_audio";
            part.text = item.value("input_audio", nlohmann::json::object()).dump();
        }
        else
        {
            continue;
        }
        message.content_parts.push_back(std::move(part));
    }
    return message;
}

ANALYSE_FILE_BEGIN

static juce::String sendRequest(juce::URL const& url, std::atomic<bool> const& shouldQuit)
{
    int statusCode = 0;
    auto const options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                             .withExtraHeaders("Content-Type: application/json")
                             .withStatusCode(&statusCode)
                             .withConnectionTimeoutMs(10000)
                             .withNumRedirectsToFollow(5)
                             .withProgressCallback([&](int, int)
                                                   {
                                                       return !shouldQuit.load();
                                                   });

    auto stream = url.createInputStream(options);
    if(stream == nullptr)
    {
        throw std::runtime_error(juce::translate("Failed to connect to remote server at RURL").replace("RURL", url.toString(true)).toStdString());
    }
    auto const responseBody = stream->readEntireStreamAsString();
    if(statusCode < 200 || statusCode >= 300)
    {
        throw std::runtime_error(juce::translate("Remote server URL returned HTTP RCODE - RMSG").replace("URL", url.toString(true)).replace("RCODE", juce::String(statusCode)).replace("RMSG", responseBody).toStdString());
    }
    return responseBody;
}

static std::optional<int32_t> getModelContextSize(juce::URL const& serverUrl, juce::String const& modelId, std::atomic<bool> const& shouldQuit)
{
    // llama.cpp servers expose the context size in the /props endpoint
    auto const propsUrl = serverUrl.withNewSubPath("/props");
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
    auto const modelsApiUrl = serverUrl.withNewSubPath("/api/v1/models");
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
    mMcpMethods.searchDocsFn = [this](std::string const& query, size_t maxNumResources)
    {
        return mRagEngine.searchDocs(query, maxNumResources);
    };
    mMcpMethods.loadDocsFn = [this](std::vector<std::string> const& ids)
    {
        return mRagEngine.loadDocs(ids);
    };
    mMcpMethods.readFilesFn = std::bind(&AgentRemote::readFiles, this, std::placeholders::_1);
    mTools = mMcpDispatcher.getToolList(mMcpMethods);
}

nlohmann::json Application::Neuralyzer::AgentRemote::readFiles(std::vector<std::string> const& filePaths)
{
    nlohmann::json response;
    response["isError"] = false;
    response["content"] = nlohmann::json::array();

    std::string message = "File loading results:\n";
    for(auto const& filePath : filePaths)
    {
        message += "\n--------\n";
        message += "Path: '" + filePath + "'\n";
        message += "Result: ";
        juce::File const file(filePath);
        if(!file.existsAsFile())
        {
            message += "The file does not exist.";
            response["isError"] = true;
        }
        else if(!file.hasReadAccess())
        {
            message += "The file does not have read access.";
            response["isError"] = true;
        }
        else if(isMediaImageFile(file))
        {
            juce::MemoryBlock block;
            if(!file.loadFileAsData(block) || block.isEmpty())
            {
                message += "Failed to read the image file.";
                response["isError"] = true;
            }
            else
            {
                auto const base64 = juce::Base64::toBase64(block.getData(), block.getSize());
                auto const dataUrl = "data:" + getImageMimeType(file) + ";base64," + base64;
                nlohmann::json imageContent;
                imageContent["type"] = "image_url";
                imageContent["image_url"]["url"] = dataUrl.toStdString();
                response["content"].push_back(imageContent);
                message += "The image file has been attached to the conversation for the multimodal model.";
            }
        }
        else if(isMediaAudioFile(file))
        {
            juce::MemoryBlock block;
            if(!file.loadFileAsData(block) || block.isEmpty())
            {
                message += "Failed to read the audio file.";
                response["isError"] = true;
            }
            else
            {
                auto const base64 = juce::Base64::toBase64(block.getData(), block.getSize());
                nlohmann::json audioContent;
                audioContent["type"] = "input_audio";
                audioContent["input_audio"]["data"] = base64.toStdString();
                audioContent["input_audio"]["format"] = getAudioFormat(file).toStdString();
                response["content"].push_back(audioContent);
                message += "The audio file has been attached to the conversation for the multimodal model.";
            }
        }
        else if(isMediaTextFile(file))
        {
            static auto constexpr maxChars = 32000;
            auto const text = file.loadFileAsString();
            auto const truncated = text.length() > maxChars;
            if(truncated)
            {
                message += "The truncated text file content (up to 32000 chars) is:\n";
                message += text.substring(0, maxChars).toStdString() + "\n...";
            }
            else
            {
                message += "The text file content is:\n";
                message += text.toStdString();
            }
        }
        else
        {
            message += "The file '" + filePath + "' is not supported by the remote agent.";
            response["isError"] = true;
        }
        message += "\n";
    }

    nlohmann::json textContent;
    textContent["type"] = "text";
    textContent["text"] = message;
    response["content"].insert(response["content"].begin(), textContent);
    return response;
}

juce::Result Application::Neuralyzer::AgentRemote::initializeModel(ModelInfo const& info)
{
    auto copy = info;
    if(!copy.serverUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid server URL: ") + copy.serverUrl.toString(true));
    }

    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.clear();
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    mContextCapacityUsage.store(0.0f);

    nlohmann::json response;
    auto const modelListUrl = copy.serverUrl.withNewSubPath("/v1/models");
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
        MiscDebug("Application::Neuralyzer::AgentRemote", "Failed to retrieve model info: " + juce::String::fromUTF8(e.what()));
        return juce::Result::fail(juce::String::fromUTF8(e.what()));
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

    copy.contextSize = getModelContextSize(copy.serverUrl, copy.modelId, mShouldQuit);

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
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.clear();
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    mContextCapacityUsage.store(0.0f);
    return juce::Result::ok();
}

std::vector<common_chat_msg> Application::Neuralyzer::AgentRemote::performInference()
{
    auto const info = getModelInfo();
    if(info.serverUrl.isEmpty() || info.modelId.isEmpty())
    {
        throw juce::translate("Remote server model not initialized");
    }

    if(!info.serverUrl.isWellFormed())
    {
        throw juce::translate("Invalid remote server URL: RSURL").replace("RSURL", info.serverUrl.toString(true));
    }

    if(mShouldQuit.load())
    {
        return {};
    }

    auto const openAiUrl = info.serverUrl.withNewSubPath("/v1/chat/completions");
    if(!openAiUrl.isWellFormed())
    {
        throw juce::translate("Invalid remote server chat URL: RSURL").replace("RSURL", openAiUrl.toString(true));
    }

    auto const history = [&]()
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        return mMessages;
    }();

    nlohmann::json request;
    request["model"] = info.modelId;
    // common_chat_msg::to_json_oaicompat() only forwards "text"/"media_marker" content parts and
    // silently drops anything else, so messages carrying inline media (see extractInlineImages)
    // are serialized manually into the OpenAI-compatible "image_url"/"input_audio" content formats.
    auto jmessages = nlohmann::json::array();
    for(auto const& msg : history)
    {
        auto const hasMedia = std::any_of(msg.content_parts.cbegin(), msg.content_parts.cend(), [](auto const& part)
                                          {
                                              return part.type == "image_url" || part.type == "input_audio";
                                          });
        if(!hasMedia)
        {
            jmessages.push_back(nlohmann::json::parse(msg.to_json_oaicompat().dump()));
            continue;
        }

        nlohmann::json jmsg;
        jmsg["role"] = msg.role;
        if(!msg.tool_name.empty())
        {
            jmsg["name"] = msg.tool_name;
        }
        if(!msg.tool_call_id.empty())
        {
            jmsg["tool_call_id"] = msg.tool_call_id;
        }
        auto content = nlohmann::json::array();
        for(auto const& part : msg.content_parts)
        {
            if(part.type == "image_url")
            {
                content.push_back({{"type", "image_url"}, {"image_url", {{"url", part.text}}}});
            }
            else if(part.type == "input_audio")
            {
                nlohmann::json inputAudio;
                try
                {
                    inputAudio = nlohmann::json::parse(part.text);
                }
                catch(...)
                {
                    continue;
                }
                content.push_back({{"type", "input_audio"}, {"input_audio", inputAudio}});
            }
            else
            {
                content.push_back({{"type", "text"}, {"text", part.text}});
            }
        }
        jmsg["content"] = content;
        jmessages.push_back(std::move(jmsg));
    }
    request["messages"] = jmessages;
    request["tools"] = common_chat_tools_to_json_oaicompat(mTools);
    MiscDebug("Application::Neuralyzer::AgentRemote", "Send: " + request.dump());

    juce::String result;
    try
    {
        result = sendRequest(openAiUrl.withPOSTData(juce::String(request.dump())), mShouldQuit);
    }
    catch(std::exception const& e)
    {
        throw juce::translate("Send request failed on remote server endpoint: ERRORSTR").replace("ERRORSTR", juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        throw juce::translate("Send request failed on remote server endpoint: ERRORSTR").replace("ERRORSTR", juce::translate("Unknown error"));
    }

    if(mShouldQuit.load())
    {
        throw juce::translate("Operation aborted.");
    }

    // Parse response
    nlohmann::json response;
    try
    {
        response = nlohmann::json::parse(result.toStdString());
    }
    catch(std::exception const& e)
    {
        throw juce::translate("Failed to parse remote server response: ERRORSTR").replace("ERRORSTR", juce::String::fromUTF8(e.what()));
    }

    MiscDebug("Application::Neuralyzer::AgentRemote", "Received response from remote server: " + juce::String(response.dump()).replace("\n", ""));

    MiscWeakAssert(response.contains("choices") && response.at("choices").is_array() && !response.at("choices").empty());
    if(!response.contains("choices") || !response.at("choices").is_array() || response.at("choices").empty())
    {
        throw juce::translate("Invalid response format from remote server: CONTENTSTR").replace("CONTENTSTR", juce::String(response.dump()));
    }

    auto choices = nlohmann::json::array();
    for(auto const& choice : response.at("choices"))
    {
        choices.push_back(choice.at("message"));
    }

    std::vector<common_chat_msg> messages;
    try
    {
        messages = common_chat_msgs_parse_oaicompat(common_json::parse(choices.dump()));
    }
    catch(std::exception const& e)
    {
        throw juce::translate("Failed to parse chat messages from response: ERRORSTR").replace("ERRORSTR", juce::String::fromUTF8(e.what()));
    }
    catch(...)
    {
        throw juce::translate("Failed to parse chat messages from response: ERRORSTR").replace("ERRORSTR", juce::translate("Unknown error"));
    }

    MiscWeakAssert(!messages.empty() && (!messages.at(0).content.empty() || !messages.at(0).tool_calls.empty()));
    if(messages.empty() || (messages.at(0).content.empty() && messages.at(0).tool_calls.empty()))
    {
        throw juce::translate("No chat messages found in response");
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

    if(mShouldQuit.load())
    {
        return {};
    }
    return messages;
}

juce::Result Application::Neuralyzer::AgentRemote::sendQuery(juce::String const& prompt)
{
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }

    common_chat_msg message;
    message.role = "user";
    message.content = prompt.toStdString();

    // Add the new message to the history
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.push_back(std::move(message));
    }

    notifyStateChanged();

    static auto constexpr maxIterations = 25_z; // Prevent infinite loops
    for(auto iteration = 0_z; iteration < maxIterations; ++iteration)
    {
        if(mShouldQuit.load())
        {
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        // Run the inference of the model using the full history
        std::vector<common_chat_msg> receivedMessages;
        try
        {
            receivedMessages = performInference();
        }
        catch(juce::String const& e)
        {
            MiscWeakAssert(false);
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - " + e);
            return juce::Result::fail(e);
        }
        catch(std::exception const& e)
        {
            MiscWeakAssert(false);
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - " + juce::String(e.what()));
            return juce::Result::fail(juce::String::fromUTF8(e.what()));
        }
        catch(...)
        {
            MiscWeakAssert(false);
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - Unknown error");
            return juce::Result::fail(juce::translate("Unknown error"));
        }

        notifyStateChanged();

        // Store the MCP tool calls and the temporary messages
        std::vector<common_chat_tool_call> toolCalls;
        std::string tempMessage;
        for(auto const& receivedMessage : receivedMessages)
        {
            toolCalls.insert(toolCalls.end(), receivedMessage.tool_calls.cbegin(), receivedMessage.tool_calls.cend());
            if(!message.reasoning_content.empty())
            {
                tempMessage += receivedMessage.reasoning_content + "\n";
            }
            if(!receivedMessage.content.empty())
            {
                tempMessage += receivedMessage.content + "\n";
            }
            else
            {
                for(auto const& contentPart : receivedMessage.content_parts)
                {
                    tempMessage += contentPart.text + "\n";
                }
            }
        }

        {
            std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
            mTempResponse = tempMessage;
        }

        // Add the received messages to the history
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.insert(mMessages.end(), receivedMessages.cbegin(), receivedMessages.cend());
        }

        notifyStateChanged();

        if(toolCalls.empty())
        {
            // There are no tool calls so the last assistant message is the final answer.
            notifyStateChanged();
            return juce::Result::ok();
        }

        // Call the MCP tools
        for(auto const& toolCall : toolCalls)
        {
            auto toolMessage = extractInlineImages(mMcpDispatcher.callTool(mMcpMethods, toolCall));
            // Add the tool message to the history
            {
                std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
                mMessages.push_back(std::move(toolMessage));
            }
        }
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted"));
    }

    // Max iterations reached - return the last response
    return juce::Result::fail(juce::translate("Maximum number of iterations reached"));
}

juce::Result Application::Neuralyzer::AgentRemote::startSession()
{
    common_chat_msg message;
    message.role = "system";
    message.content = mMcpDispatcher.getInstructions();

    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages = {std::move(message)};
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
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

    auto const messages = [this]()
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        return mMessages;
    }();

    try
    {
        nlohmann::json root;
        root["version"] = mMcpDispatcher.getUuid();
        for(auto const& message : messages)
        {
            root["messages"].push_back(nlohmann::json::parse(message.to_json_oaicompat().dump()));
        }

        if(!sessionFile.replaceWithText(juce::String(root.dump(2))))
        {
            return juce::Result::fail(juce::translate("Failed to write session file"));
        }

        MiscDebug("Application::Neuralyzer::AgentRemote", "Session saved with " + juce::String(messages.size()) + " messages");
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

    std::vector<common_chat_msg> messages;
    try
    {
        auto const root = nlohmann::json::parse(sessionFile.loadFileAsString().toStdString());
        if(!root.contains("messages") || !root.at("messages").is_array())
        {
            return juce::Result::fail(juce::translate("Invalid message state file format"));
        }

        messages = common_chat_msgs_parse_oaicompat(common_json::parse(root.at("messages").dump()));
    }
    catch(std::exception const& e)
    {
        return juce::Result::fail(juce::translate("Failed to load session: ") + juce::String(e.what()));
    }

    // Use the latest instructions
    messages.front().content = mMcpDispatcher.getInstructions();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages = std::move(messages);
    }

    notifyStateChanged();
    MiscDebug("Application::Neuralyzer::AgentRemote", "Session loaded " + sessionFile.getFullPathName());
    return juce::Result::ok();
}

std::vector<common_chat_msg> Application::Neuralyzer::AgentRemote::getHistory() const
{
    std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
    return mMessages;
}

juce::String Application::Neuralyzer::AgentRemote::getTemporaryResponse() const
{
    std::unique_lock<std::mutex> sessionLock(mTemporaryMutex);
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
    auto callback = [this]()
    {
        std::unique_lock<std::mutex> callbackLock(mCallbackMutex);
        return mNotifyCallback;
    }();

    if(callback != nullptr)
    {
        callback();
    }
}

ANALYSE_FILE_END
