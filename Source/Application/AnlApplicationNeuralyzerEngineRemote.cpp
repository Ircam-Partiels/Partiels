#include "AnlApplicationNeuralyzerEngineRemote.h"

ANALYSE_FILE_BEGIN

// The timeout of the requests that list the models and retrieve their properties
static auto constexpr requestTimeoutMs = 30000;
// The timeout of the chat completions: the server sends nothing while it evaluates the prompt, which can be long with a
// long history or with media (the timeout applies to the inactivity of the connection on most platforms)
static auto constexpr generationTimeoutMs = 120000;

// Gets the URL of an endpoint of the server: the server URL can contain a base path (e.g. a reverse proxy) and can end
// with the version of the API that is already part of the endpoint paths
static juce::URL getEndpointUrl(juce::URL const& serverUrl, juce::String const& endpointPath)
{
    auto baseUrl = serverUrl.toString(false).trimCharactersAtEnd("/");
    if(baseUrl.endsWithIgnoreCase("/v1"))
    {
        baseUrl = baseUrl.dropLastCharacters(3);
    }
    return juce::URL(baseUrl + endpointPath);
}

// Cancels a stream from a separate thread when the abort flag is set, since the connection and the reading of the stream
// block while the server evaluates the prompt
class StreamCanceller
{
public:
    StreamCanceller(juce::WebInputStream& stream, std::atomic<bool> const& shouldAbort)
    : mStream(stream)
    , mShouldAbort(shouldAbort)
    , mThread([this]()
              {
                  std::unique_lock<std::mutex> lock(mMutex);
                  while(!mCondition.wait_for(lock, std::chrono::milliseconds(50), [this]()
                                             {
                                                 return mIsDone;
                                             }))
                  {
                      if(mShouldAbort.load())
                      {
                          mStream.cancel();
                          return;
                      }
                  }
              })
    {
    }

    ~StreamCanceller()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mIsDone = true;
        }
        mCondition.notify_one();
        mThread.join();
    }

private:
    juce::WebInputStream& mStream;
    std::atomic<bool> const& mShouldAbort;
    std::mutex mMutex;
    std::condition_variable mCondition;
    bool mIsDone{false};
    std::thread mThread; // Must be the last member as it uses the other members
};

// Sends a request and reads the response if the server accepts the request (the request is silently interrupted if it
// is aborted)
static void sendRequest(juce::URL const& url, bool usePost, int timeoutMs, std::atomic<bool> const& shouldAbort, std::function<void(juce::WebInputStream&)> const& readResponse)
{
    juce::WebInputStream stream(url, usePost);
    stream.withExtraHeaders("Content-Type: application/json").withConnectionTimeout(timeoutMs).withNumRedirectsToFollow(5);
    StreamCanceller const canceller(stream, shouldAbort);
    auto const isConnected = stream.connect(nullptr);
    if(shouldAbort.load())
    {
        return;
    }
    auto const statusCode = stream.getStatusCode();
    if(!isConnected && statusCode == 0)
    {
        throw juce::translate("Failed to connect to remote server at RURL").replace("RURL", url.toString(false));
    }
    if(statusCode < 200 || statusCode >= 300)
    {
        throw juce::translate("Remote server RURL returned HTTP RCODE - RMSG").replace("RURL", url.toString(false)).replace("RCODE", juce::String(statusCode)).replace("RMSG", stream.readEntireStreamAsString());
    }
    readResponse(stream);
}

// Sends a GET request and returns the response
static juce::String getRequest(juce::URL const& url, std::atomic<bool> const& shouldAbort)
{
    juce::String response;
    sendRequest(url, false, requestTimeoutMs, shouldAbort, [&](juce::WebInputStream& stream)
                {
                    response = stream.readEntireStreamAsString();
                });
    return response;
}

// Reads the lines of a stream as they arrive (the callback returns false to stop the reading)
static void readLines(juce::InputStream& stream, std::atomic<bool> const& shouldAbort, std::function<bool(juce::String const&)> const& callback)
{
    juce::MemoryOutputStream line;
    char character;
    while(!shouldAbort.load() && stream.read(&character, 1) == 1)
    {
        if(character == '\n')
        {
            auto const text = line.toUTF8().trimCharactersAtEnd("\r");
            line.reset();
            if(!callback(text))
            {
                return;
            }
        }
        else
        {
            line.writeByte(character);
        }
    }
    if(!shouldAbort.load() && line.getDataSize() > 0)
    {
        callback(line.toUTF8().trimCharactersAtEnd("\r"));
    }
}

static nlohmann::json parseResponse(juce::String const& response)
{
    try
    {
        return nlohmann::json::parse(response.toStdString());
    }
    catch(std::exception const& e)
    {
        throw juce::translate("Failed to parse remote server response: ERRORSTR").replace("ERRORSTR", juce::String::fromUTF8(e.what()));
    }
}

// Gets the models listed by the server: the OpenAI-compatible endpoint is /v1/models but some servers only expose /models
static nlohmann::json getModelList(juce::URL const& serverUrl, std::atomic<bool> const& shouldAbort)
{
    juce::String error;
    for(auto const* endpointPath : {"/v1/models", "/models"})
    {
        try
        {
            auto const response = parseResponse(getRequest(getEndpointUrl(serverUrl, endpointPath), shouldAbort));
            if(response.contains("data") && response.at("data").is_array())
            {
                return response.at("data");
            }
            error = juce::translate("Invalid model list response format");
        }
        catch(juce::String const& e)
        {
            error = e;
        }
        if(shouldAbort.load())
        {
            return nlohmann::json::array();
        }
    }
    throw error;
}

// Gets the model that supports images with the /props endpoint of llama.cpp servers: the versions of llama.cpp before
// b11938 do not report the input modalities in the model list (the server runs a single model)
static std::optional<std::string> getVisionModel(juce::URL const& serverUrl, std::atomic<bool> const& shouldAbort)
{
    try
    {
        auto const response = parseResponse(getRequest(getEndpointUrl(serverUrl, "/props"), shouldAbort));
        if(response.contains("modalities") && response.at("modalities").is_object() && response.at("modalities").value("vision", false) && response.contains("model_alias") && response.at("model_alias").is_string())
        {
            return response.at("model_alias").get<std::string>();
        }
    }
    catch([[maybe_unused]] juce::String const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve the modalities from /props: " + e);
    }
    catch([[maybe_unused]] std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve the modalities from /props: " + juce::String(e.what()));
    }
    return std::nullopt;
}

static std::optional<int32_t> getModelContextSize(juce::URL const& serverUrl, juce::String const& modelId, std::atomic<bool> const& shouldAbort)
{
    // Llama.cpp servers expose the context size in the /props endpoint
    try
    {
        auto const response = parseResponse(getRequest(getEndpointUrl(serverUrl, "/props"), shouldAbort));
        if(response.contains("default_generation_settings") && response.at("default_generation_settings").is_object())
        {
            auto const contextSize = response.at("default_generation_settings").value("n_ctx", 0);
            if(contextSize > 0)
            {
                return contextSize;
            }
        }
    }
    catch([[maybe_unused]] juce::String const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve context size from /props: " + e);
    }
    catch([[maybe_unused]] std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve context size from /props: " + juce::String(e.what()));
    }

    // LM Studio servers expose the context size in the /api/v1/models endpoint
    try
    {
        auto const response = parseResponse(getRequest(getEndpointUrl(serverUrl, "/api/v1/models"), shouldAbort));
        if(response.contains("models") && response.at("models").is_array())
        {
            auto const& models = response.at("models");
            auto const modelIt = std::find_if(models.cbegin(), models.cend(), [&](auto const& model)
                                              {
                                                  return model.is_object() && model.value("key", std::string{}) == modelId.toStdString();
                                              });
            if(modelIt != models.cend() && modelIt->contains("loaded_instances") && modelIt->at("loaded_instances").is_array())
            {
                for(auto const& instance : modelIt->at("loaded_instances"))
                {
                    if(instance.contains("config") && instance.at("config").is_object())
                    {
                        auto const contextSize = instance.at("config").value("context_length", 0);
                        if(contextSize > 0)
                        {
                            return contextSize;
                        }
                    }
                }
            }
        }
    }
    catch([[maybe_unused]] juce::String const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve context size from /api/v1/models: " + e);
    }
    catch([[maybe_unused]] std::exception const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve context size from /api/v1/models: " + juce::String(e.what()));
    }
    return std::nullopt;
}

// Converts the messages to the OpenAI-compatible format: common_chat_msg::to_json_oaicompat() only forwards the "text"
// and "media_marker" content parts, so the messages with media are converted to the "image_url" and "input_audio"
// content formats
static nlohmann::json toJsonMessages(std::vector<common_chat_msg> const& messages)
{
    auto jsonMessages = nlohmann::json::array();
    for(auto const& message : messages)
    {
        auto const hasMedia = std::any_of(message.content_parts.cbegin(), message.content_parts.cend(), [](auto const& part)
                                          {
                                              return part.type == "image_url" || part.type == "input_audio";
                                          });
        if(!hasMedia)
        {
            jsonMessages.push_back(nlohmann::json::parse(message.to_json_oaicompat().dump()));
            continue;
        }

        nlohmann::json jsonMessage;
        jsonMessage["role"] = message.role;
        if(!message.tool_name.empty())
        {
            jsonMessage["name"] = message.tool_name;
        }
        if(!message.tool_call_id.empty())
        {
            jsonMessage["tool_call_id"] = message.tool_call_id;
        }
        auto content = nlohmann::json::array();
        for(auto const& part : message.content_parts)
        {
            if(part.type == "image_url")
            {
                content.push_back({{"type", "image_url"}, {"image_url", {{"url", part.text}}}});
            }
            else if(part.type == "input_audio")
            {
                // The data and the format are stored as JSON in the text of the content part
                try
                {
                    content.push_back({{"type", "input_audio"}, {"input_audio", nlohmann::json::parse(part.text)}});
                }
                catch(...)
                {
                    MiscWeakAssert(false);
                }
            }
            else
            {
                content.push_back({{"type", "text"}, {"text", part.text}});
            }
        }
        jsonMessage["content"] = content;
        jsonMessages.push_back(std::move(jsonMessage));
    }
    return jsonMessages;
}

// Accumulates the chunks of a streamed chat completion (server-sent events) into a message. A response that is not
// streamed (if the server ignores the stream option) is also supported.
class ChatStream
{
public:
    // Processes a line of the stream and returns false when the stream is complete (throws juce::String on error)
    bool processLine(juce::String const& line)
    {
        // The empty lines separate the events, the lines starting with a colon are comments (keep-alive) and the other
        // fields of the events are not used
        auto const text = line.trim();
        if(text.isEmpty() || text.startsWithChar(':') || text.startsWith("event:") || text.startsWith("id:") || text.startsWith("retry:"))
        {
            return true;
        }
        if(!text.startsWith("data:"))
        {
            mBody << line << "\n";
            return true;
        }
        auto const data = text.substring(5).trimStart();
        if(data == "[DONE]")
        {
            return false;
        }
        processChunk(parseResponse(data));
        return true;
    }

    // Gets whether the server has started to send the response
    bool hasStarted() const
    {
        return mHasStarted;
    }

    // Gets the reasoning and the content received so far
    std::string getText() const
    {
        return mReasoning.empty() ? mContent : mReasoning + "\n" + mContent;
    }

    // Gets the number of tokens used by the prompt and the response (if reported by the server)
    std::optional<int64_t> getTotalTokens() const
    {
        return mTotalTokens;
    }

    // Gets the message (throws juce::String on error)
    common_chat_msg finish()
    {
        if(!mHasStarted && mBody.isNotEmpty())
        {
            processChunk(parseResponse(mBody));
        }

        common_chat_msg message;
        message.role = "assistant";
        message.content = mContent;
        message.reasoning_content = mReasoning;
        for(auto const& toolCall : mToolCalls)
        {
            common_chat_tool_call call;
            call.id = toolCall.id;
            call.name = toolCall.name;
            call.arguments = toolCall.arguments.empty() ? "{}" : toolCall.arguments;
            message.tool_calls.push_back(std::move(call));
        }
        return message;
    }

private:
    struct ToolCall
    {
        std::string id;
        std::string name;
        std::string arguments;
    };

    void processChunk(nlohmann::json const& chunk)
    {
        if(!chunk.is_object())
        {
            return;
        }
        if(chunk.contains("error"))
        {
            auto const& error = chunk.at("error");
            auto const message = error.is_string() ? error.get<std::string>() : (error.is_object() ? error.value("message", error.dump()) : error.dump());
            throw juce::translate("Remote server error: ERRORSTR").replace("ERRORSTR", juce::String::fromUTF8(message.c_str()));
        }
        if(chunk.contains("usage") && chunk.at("usage").is_object())
        {
            auto const totalTokens = chunk.at("usage").value("total_tokens", static_cast<int64_t>(0));
            if(totalTokens > 0)
            {
                mTotalTokens = totalTokens;
            }
        }
        if(!chunk.contains("choices") || !chunk.at("choices").is_array() || chunk.at("choices").empty())
        {
            return;
        }

        // Only one choice is requested
        auto const& choice = chunk.at("choices").front();
        mHasStarted = true;
        if(choice.contains("delta") && choice.at("delta").is_object())
        {
            processDelta(choice.at("delta"));
        }
        else if(choice.contains("message") && choice.at("message").is_object())
        {
            processDelta(choice.at("message"));
        }
    }

    void processDelta(nlohmann::json const& delta)
    {
        if(delta.contains("content") && delta.at("content").is_string())
        {
            mContent += delta.at("content").get<std::string>();
        }
        // The servers use either reasoning_content or reasoning
        for(auto const* key : {"reasoning_content", "reasoning"})
        {
            if(delta.contains(key) && delta.at(key).is_string())
            {
                mReasoning += delta.at(key).get<std::string>();
            }
        }
        if(!delta.contains("tool_calls") || !delta.at("tool_calls").is_array())
        {
            return;
        }
        for(auto const& toolCallDelta : delta.at("tool_calls"))
        {
            if(!toolCallDelta.is_object())
            {
                continue;
            }
            // The fragments of a tool call share the same index (the identifier and the name are only in the first
            // fragment). Without index, a tool call with an identifier starts a new call and the others complete the
            // last call.
            auto const index = [&]()
            {
                if(toolCallDelta.contains("index") && toolCallDelta.at("index").is_number_unsigned())
                {
                    return std::min(toolCallDelta.at("index").get<size_t>(), mToolCalls.size());
                }
                return toolCallDelta.contains("id") || mToolCalls.empty() ? mToolCalls.size() : mToolCalls.size() - 1_z;
            }();
            if(index == mToolCalls.size())
            {
                mToolCalls.emplace_back();
            }
            auto& toolCall = mToolCalls[index];
            if(toolCallDelta.contains("id") && toolCallDelta.at("id").is_string() && !toolCallDelta.at("id").get<std::string>().empty())
            {
                toolCall.id = toolCallDelta.at("id").get<std::string>();
            }
            if(toolCallDelta.contains("function") && toolCallDelta.at("function").is_object())
            {
                auto const& function = toolCallDelta.at("function");
                if(function.contains("name") && function.at("name").is_string() && !function.at("name").get<std::string>().empty())
                {
                    toolCall.name = function.at("name").get<std::string>();
                }
                if(function.contains("arguments"))
                {
                    auto const& arguments = function.at("arguments");
                    if(arguments.is_string())
                    {
                        toolCall.arguments += arguments.get<std::string>();
                    }
                    else if(!arguments.is_null())
                    {
                        toolCall.arguments = arguments.dump();
                    }
                }
            }
        }
    }

    juce::String mBody;
    bool mHasStarted{false};
    std::string mContent;
    std::string mReasoning;
    std::vector<ToolCall> mToolCalls;
    std::optional<int64_t> mTotalTokens;
};

Application::Neuralyzer::EngineRemote::EngineRemote(std::atomic<bool> const& shouldAbort)
: mShouldAbort(shouldAbort)
{
}

juce::Result Application::Neuralyzer::EngineRemote::initialize(ModelInfo const& info)
{
    reset();
    if(!info.serverUrl.isWellFormed())
    {
        return juce::Result::fail(juce::translate("Invalid remote server URL: RSURL").replace("RSURL", info.serverUrl.toString(false)));
    }

    nlohmann::json models;
    try
    {
        models = getModelList(info.serverUrl, mShouldAbort);
    }
    catch(juce::String const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve the model list: " + e);
        return juce::Result::fail(e);
    }
    if(mShouldAbort.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted"));
    }

    auto const modelIt = std::find_if(models.cbegin(), models.cend(), [&](auto const& model)
                                      {
                                          return model.is_object() && model.value("id", std::string{}) == info.modelId.toStdString();
                                      });
    if(modelIt == models.cend())
    {
        return juce::Result::fail(juce::translate("Model MODELID not found in model list response.").replace("MODELID", info.modelId));
    }

    auto copy = info;
    copy.contextSize = getModelContextSize(info.serverUrl, info.modelId, mShouldAbort);
    {
        std::lock_guard<std::mutex> lock(mModelInfoMutex);
        mModelInfo = copy;
    }
    MiscDebug("Application::Neuralyzer::EngineRemote", "Initialized with model: " + info.modelId + " at " + info.serverUrl.toString(false));
    return juce::Result::ok();
}

void Application::Neuralyzer::EngineRemote::reset()
{
    mContextUsage.store(0.0f);
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    mModelInfo = {};
}

bool Application::Neuralyzer::EngineRemote::isInitialized() const
{
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    return mModelInfo.modelId.isNotEmpty();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::EngineRemote::getModelInfo() const
{
    std::lock_guard<std::mutex> lock(mModelInfoMutex);
    return mModelInfo;
}

float Application::Neuralyzer::EngineRemote::getContextUsage() const
{
    return mContextUsage.load();
}

common_chat_msg Application::Neuralyzer::EngineRemote::generate(std::vector<common_chat_msg> const& messages, std::vector<common_chat_tool> const& tools, bool useThinkingMode, std::function<void()> onGenerationStarted, std::function<void(std::string const&)> onResponseChanged)
{
    auto const info = getModelInfo();
    if(info.modelId.isEmpty())
    {
        throw juce::translate("The model is not initialized.");
    }
    if(mShouldAbort.load())
    {
        return {};
    }

    nlohmann::json request;
    request["model"] = info.modelId.toStdString();
    request["messages"] = toJsonMessages(messages);
    if(!tools.empty())
    {
        // The common_json of llama.cpp is converted with its text since it is implicitly convertible to a string
        request["tools"] = nlohmann::json::parse(common_chat_tools_to_json_oaicompat(tools).dump());
    }
    request["chat_template_kwargs"] = {{"enable_thinking", useThinkingMode}};
    request["stream"] = true;
    request["stream_options"] = {{"include_usage", true}};
    auto const body = request.dump();
    // The request is not logged since the media are embedded in base64
    MiscDebug("Application::Neuralyzer::EngineRemote", "Send " + juce::String(messages.size()) + " messages (" + juce::String(body.size()) + " bytes)");

    ChatStream stream;
    auto const url = getEndpointUrl(info.serverUrl, "/v1/chat/completions").withPOSTData(juce::MemoryBlock(body.data(), body.size()));
    sendRequest(url, true, generationTimeoutMs, mShouldAbort, [&](juce::WebInputStream& input)
                {
                    readLines(input, mShouldAbort, [&](juce::String const& line)
                              {
                                  auto const hadStarted = stream.hasStarted();
                                  auto const isPending = stream.processLine(line);
                                  if(!hadStarted && stream.hasStarted() && onGenerationStarted != nullptr)
                                  {
                                      onGenerationStarted();
                                  }
                                  if(onResponseChanged != nullptr)
                                  {
                                      onResponseChanged(stream.getText());
                                  }
                                  return isPending;
                              });
                });
    if(mShouldAbort.load())
    {
        return {};
    }

    auto message = stream.finish();
    auto const totalTokens = stream.getTotalTokens();
    auto const contextSize = info.contextSize.value_or(0);
    if(totalTokens.has_value() && contextSize > 0)
    {
        mContextUsage.store(static_cast<float>(static_cast<double>(totalTokens.value()) / static_cast<double>(contextSize)));
    }
    MiscDebug("Application::Neuralyzer::EngineRemote", "Response: " + juce::String::fromUTF8(stream.getText().c_str()) + " (" + juce::String(message.tool_calls.size()) + " tool calls)");
    return message;
}

std::set<std::pair<juce::String, bool>> Application::Neuralyzer::EngineRemote::getAvailableModels(juce::URL const& serverUrl)
{
    if(serverUrl.isEmpty() || !serverUrl.isWellFormed())
    {
        return {};
    }

    static std::atomic<bool> const shouldAbort{false};
    nlohmann::json models;
    try
    {
        models = getModelList(serverUrl, shouldAbort);
    }
    catch([[maybe_unused]] juce::String const& e)
    {
        MiscDebug("Application::Neuralyzer::EngineRemote", "Failed to retrieve the model list: " + e);
        return {};
    }

    auto const hasInputModalities = [](nlohmann::json const& model)
    {
        return model.contains("architecture") && model.at("architecture").contains("input_modalities") && model.at("architecture").at("input_modalities").is_array();
    };
    // The /props endpoint is only requested if a model does not report its input modalities
    auto const needsProps = std::any_of(models.cbegin(), models.cend(), [&](auto const& model)
                                        {
                                            return model.is_object() && !hasInputModalities(model);
                                        });
    auto const visionModel = needsProps ? getVisionModel(serverUrl, shouldAbort) : std::nullopt;

    std::set<std::pair<juce::String, bool>> availableModels;
    for(auto const& model : models)
    {
        if(!model.is_object() || !model.contains("id") || !model.at("id").is_string())
        {
            continue;
        }
        auto const id = model.at("id").get<std::string>();
        auto const supportsImage = [&]()
        {
            if(!hasInputModalities(model))
            {
                return visionModel.has_value() && visionModel.value() == id;
            }
            auto const& modalities = model.at("architecture").at("input_modalities");
            return std::find(modalities.cbegin(), modalities.cend(), "image") != modalities.cend();
        }();
        availableModels.insert(std::make_pair(juce::String::fromUTF8(id.c_str()), supportsImage));
    }
    return availableModels;
}

class EngineRemoteTest
: public juce::UnitTest
{
public:
    EngineRemoteTest()
    : juce::UnitTest("Neuralyzer Engine Remote", "Application")
    {
    }

    ~EngineRemoteTest() override = default;

    void runTest() override
    {
        // Processes the lines of a stream and returns the message
        auto const processStream = [](std::vector<juce::String> const& lines)
        {
            ChatStream stream;
            for(auto const& line : lines)
            {
                if(!stream.processLine(line))
                {
                    break;
                }
            }
            return stream.finish();
        };

        beginTest("Endpoint URL");
        {
            auto const getEndpoint = [](juce::String const& serverUrl)
            {
                return getEndpointUrl(juce::URL(serverUrl), "/v1/models").toString(false);
            };
            expectEquals(getEndpoint("http://localhost:1234"), juce::String("http://localhost:1234/v1/models"));
            expectEquals(getEndpoint("http://localhost:1234/"), juce::String("http://localhost:1234/v1/models"));
            expectEquals(getEndpoint("http://localhost:1234/v1"), juce::String("http://localhost:1234/v1/models"));
            expectEquals(getEndpoint("http://localhost:1234/v1/"), juce::String("http://localhost:1234/v1/models"));
            expectEquals(getEndpoint("https://example.com/llm"), juce::String("https://example.com/llm/v1/models"));
            expectEquals(getEndpoint("https://example.com/llm/v1"), juce::String("https://example.com/llm/v1/models"));
        }

        beginTest("Stream Content");
        {
            auto const message = processStream({
                R"(data: {"choices":[{"index":0,"delta":{"role":"assistant","content":null}}]})",
                "",
                ": keep-alive",
                R"(data: {"choices":[{"index":0,"delta":{"reasoning_content":"Let me "}}]})",
                R"(data: {"choices":[{"index":0,"delta":{"reasoning_content":"think."}}]})",
                R"(data:{"choices":[{"index":0,"delta":{"content":"Hello"}}]})",
                R"(data: {"choices":[{"index":0,"delta":{"content":" world"},"finish_reason":"stop"}]})",
                R"(data: {"choices":[],"usage":{"prompt_tokens":10,"completion_tokens":5,"total_tokens":15}})",
                "data: [DONE]",
                R"(data: {"choices":[{"index":0,"delta":{"content":" ignored"}}]})",
            });
            expectEquals(juce::String(message.role), juce::String("assistant"));
            expectEquals(juce::String(message.content), juce::String("Hello world"));
            expectEquals(juce::String(message.reasoning_content), juce::String("Let me think."));
            expect(message.tool_calls.empty());
        }

        beginTest("Stream Reasoning Field");
        {
            auto const message = processStream({
                R"(data: {"choices":[{"delta":{"reasoning":"Hmm"}}]})",
                R"(data: {"choices":[{"delta":{"content":"Yes"}}]})",
            });
            expectEquals(juce::String(message.reasoning_content), juce::String("Hmm"));
            expectEquals(juce::String(message.content), juce::String("Yes"));
        }

        beginTest("Stream Usage");
        {
            ChatStream stream;
            expect(!stream.hasStarted());
            expect(stream.processLine(R"(data: {"choices":[{"delta":{"content":"A"}}]})"));
            expect(stream.hasStarted());
            expect(!stream.getTotalTokens().has_value());
            expect(stream.processLine(R"(data: {"choices":[],"usage":{"total_tokens":42}})"));
            expect(stream.getTotalTokens().has_value() && stream.getTotalTokens().value() == 42);
            expect(!stream.processLine("data: [DONE]"));
        }

        beginTest("Stream Tool Calls");
        {
            auto const message = processStream({
                R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"id":"call_1","type":"function","function":{"name":"first","arguments":""}}]}}]})",
                R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"{\"a\":"}}]}}]})",
                R"(data: {"choices":[{"delta":{"tool_calls":[{"index":1,"id":"call_2","type":"function","function":{"name":"second","arguments":"{}"}}]}}]})",
                R"(data: {"choices":[{"delta":{"tool_calls":[{"index":0,"function":{"arguments":"1}"}}]}}]})",
                R"(data: {"choices":[{"delta":{"tool_calls":[{"id":"call_3","type":"function","function":{"name":"third"}}]}}]})",
                R"(data: {"choices":[{"delta":{},"finish_reason":"tool_calls"}]})",
                "data: [DONE]",
            });
            expectEquals(message.tool_calls.size(), 3_z);
            if(message.tool_calls.size() == 3_z)
            {
                expectEquals(juce::String(message.tool_calls[0].id), juce::String("call_1"));
                expectEquals(juce::String(message.tool_calls[0].name), juce::String("first"));
                expectEquals(juce::String(message.tool_calls[0].arguments), juce::String(R"({"a":1})"));
                expectEquals(juce::String(message.tool_calls[1].id), juce::String("call_2"));
                expectEquals(juce::String(message.tool_calls[1].name), juce::String("second"));
                expectEquals(juce::String(message.tool_calls[1].arguments), juce::String("{}"));
                expectEquals(juce::String(message.tool_calls[2].id), juce::String("call_3"));
                expectEquals(juce::String(message.tool_calls[2].name), juce::String("third"));
                expectEquals(juce::String(message.tool_calls[2].arguments), juce::String("{}"));
            }
        }

        beginTest("Stream Error");
        {
            auto const getError = [&](std::vector<juce::String> const& lines)
            {
                try
                {
                    processStream(lines);
                }
                catch(juce::String const& e)
                {
                    return e;
                }
                return juce::String{};
            };
            expect(getError({R"(data: {"choices":[{"delta":{"content":"A"}}]})", R"(data: {"error":{"message":"Context size exceeded","code":400}})"}).contains("Context size exceeded"));
            expect(getError({"event: error", R"(data: {"error":"Server overloaded"})"}).contains("Server overloaded"));
            expect(getError({"data: {invalid"}).isNotEmpty());
            expect(getError({"not a json response"}).isNotEmpty());
        }

        beginTest("Response Not Streamed");
        {
            auto const message = processStream({
                "{",
                R"(  "choices": [{"index": 0, "message": {"role": "assistant", "content": "Done", "tool_calls": [{"id": "call_1", "type": "function", "function": {"name": "tool", "arguments": {"x": 2}}}]}}],)",
                R"(  "usage": {"total_tokens": 7})",
                "}",
            });
            expectEquals(juce::String(message.content), juce::String("Done"));
            expectEquals(message.tool_calls.size(), 1_z);
            if(message.tool_calls.size() == 1_z)
            {
                expectEquals(juce::String(message.tool_calls[0].name), juce::String("tool"));
                expectEquals(juce::String(message.tool_calls[0].arguments), juce::String(R"({"x":2})"));
            }
        }

        beginTest("Media Messages");
        {
            common_chat_msg textMessage;
            textMessage.role = "user";
            textMessage.content = "Hello";

            common_chat_msg mediaMessage;
            mediaMessage.role = "tool";
            mediaMessage.tool_name = "read_files";
            mediaMessage.tool_call_id = "call_1";
            common_chat_msg_content_part textPart;
            textPart.type = "text";
            textPart.text = "File loading results";
            mediaMessage.content_parts.push_back(textPart);
            common_chat_msg_content_part imagePart;
            imagePart.type = "image_url";
            imagePart.text = "data:image/png;base64,AAAA";
            mediaMessage.content_parts.push_back(imagePart);
            common_chat_msg_content_part audioPart;
            audioPart.type = "input_audio";
            audioPart.text = R"({"data":"BBBB","format":"wav"})";
            mediaMessage.content_parts.push_back(audioPart);

            auto const jsonMessages = toJsonMessages({textMessage, mediaMessage});
            expectEquals(jsonMessages.size(), 2_z);
            if(jsonMessages.size() == 2_z)
            {
                expect(jsonMessages[0].at("content") == "Hello");
                auto const& media = jsonMessages[1];
                expect(media.at("role") == "tool");
                expect(media.at("name") == "read_files");
                expect(media.at("tool_call_id") == "call_1");
                expect(media.at("content").is_array() && media.at("content").size() == 3_z);
                if(media.at("content").is_array() && media.at("content").size() == 3_z)
                {
                    expect(media.at("content")[0] == nlohmann::json({{"type", "text"}, {"text", "File loading results"}}));
                    expect(media.at("content")[1] == nlohmann::json({{"type", "image_url"}, {"image_url", {{"url", "data:image/png;base64,AAAA"}}}}));
                    expect(media.at("content")[2] == nlohmann::json({{"type", "input_audio"}, {"input_audio", {{"data", "BBBB"}, {"format", "wav"}}}}));
                }
            }
        }
    }
};

static EngineRemoteTest engineRemoteTest;

ANALYSE_FILE_END
