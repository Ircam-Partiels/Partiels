#include "AnlApplicationNeuralyzerAgentRemote.h"
#include "AnlApplicationInstance.h"
#include <AnlResourceData.h>

static bool isMediaImageFile(juce::File const& file)
{
    return file.hasFileExtension("png;jpg;jpeg;gif;webp");
}

// The audio formats decoded by the multimodal projectors of llama.cpp (mtmd)
static bool isMediaAudioFile(juce::File const& file)
{
    return file.hasFileExtension("wav;mp3;flac");
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

// The OpenAI-compatible "input_audio" content part only documents "wav" and "mp3" as formats, but llama.cpp detects the
// format from the data and also decodes "flac".
static juce::String getAudioFormat(juce::File const& file)
{
    auto const extension = file.getFileExtension().toLowerCase();
    return extension.isEmpty() ? juce::String("wav") : extension.substring(1);
}

// Extracts the image_url/input_audio content parts produced by readFiles() into their own content
// parts, since some OpenAI-compatible endpoints require the media to be described as a structured
// content array rather than being flattened into a plain string like the rest of the payload.
static common_chat_msg extractInlineMedia(common_chat_msg const& toolMessage)
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

std::set<std::pair<juce::String, bool>> Application::Neuralyzer::AgentRemote::getAvailableModels(juce::URL const& serverUrl)
{
    return EngineRemote::getAvailableModels(serverUrl);
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
    mMcpMethods.readFilesFn = [this](std::vector<std::string> const& filePaths)
    {
        return readFiles(filePaths);
    };
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
        // The path is checked before creating the file, which requires an absolute path
        if(!juce::File::isAbsolutePath(juce::String::fromUTF8(filePath.c_str())))
        {
            message += "The path is not absolute.\n";
            response["isError"] = true;
            continue;
        }
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
            // The size of the audio file is not limited, but the file is kept in base64 in the history and sent again
            // with each request (about 11 MB per minute for a stereo WAV file at 48 kHz and 16 bits). In practice, the
            // duration of the audio is limited by the context size of the model.
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
    mEngine.reset();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.clear();
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    notifyStateChanged();
    auto const result = mEngine.initialize(info);
    notifyStateChanged();
    return result;
}

juce::Result Application::Neuralyzer::AgentRemote::resetModel()
{
    mEngine.reset();
    {
        std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
        mMessages.clear();
    }
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }
    notifyStateChanged();
    return juce::Result::ok();
}

common_chat_msg Application::Neuralyzer::AgentRemote::performInference(bool useThinkingMode)
{
    return mEngine.generate(getHistory(), mTools, useThinkingMode, [this]()
                            {
                                notifyStateChanged();
                            },
                            [this](std::string const& response)
                            {
                                std::unique_lock<std::mutex> lock(mTemporaryMutex);
                                mTempResponse = response;
                            });
}

juce::Result Application::Neuralyzer::AgentRemote::sendQuery(juce::String const& prompt, bool useThinkingMode)
{
    {
        std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
        mTempResponse.clear();
    }

    if(!mEngine.isInitialized())
    {
        MiscDebug("Application::Neuralyzer::AgentRemote", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    common_chat_msg message;
    message.role = "user";
    message.content = prompt.toStdString();

    // Keep the history to restore it if the query fails: the restore point follows the completed tool calls so the
    // history stays consistent with the modifications of the document
    auto restorePoint = getHistory();
    auto const restoreHistory = [&]()
    {
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages = restorePoint;
        }
        notifyStateChanged();
    };

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
            restoreHistory();
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        common_chat_msg response;
        try
        {
            response = performInference(useThinkingMode);
        }
        catch(juce::String const& e)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - " + e);
            return juce::Result::fail(e);
        }
        catch(std::exception const& e)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - " + juce::String(e.what()));
            return juce::Result::fail(juce::String::fromUTF8(e.what()));
        }
        catch(...)
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - Unknown error");
            return juce::Result::fail(juce::translate("Unknown error"));
        }

        if(mShouldQuit.load())
        {
            restoreHistory();
            return juce::Result::fail(juce::translate("Operation aborted"));
        }

        if(response.empty())
        {
            restoreHistory();
            MiscDebug("Application::Neuralyzer::AgentRemote", "Inference failed - No model response");
            return juce::Result::fail(juce::translate("No model response"));
        }

        auto const toolCalls = response.tool_calls;
        {
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.push_back(std::move(response));
        }
        notifyStateChanged();

        // Without tool calls, the response is the final answer
        if(toolCalls.empty())
        {
            return juce::Result::ok();
        }

        for(auto const& toolCall : toolCalls)
        {
            auto toolMessage = extractInlineMedia(mMcpDispatcher.callTool(mMcpMethods, toolCall));
            std::unique_lock<std::mutex> sessionLock(mMessagesMutex);
            mMessages.push_back(std::move(toolMessage));
        }
        restorePoint = getHistory();
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted"));
    }
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
    notifyStateChanged();
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

    auto const messages = getHistory();
    try
    {
        nlohmann::json root;
        root["version"] = mMcpDispatcher.getUuid();
        root["messages"] = nlohmann::json::array();
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

    // Use the latest instructions (the system message is restored if missing)
    if(messages.empty() || messages.front().role != "system")
    {
        common_chat_msg systemMessage;
        systemMessage.role = "system";
        messages.insert(messages.begin(), std::move(systemMessage));
    }
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
    std::unique_lock<std::mutex> temporaryLock(mTemporaryMutex);
    return juce::String::fromUTF8(mTempResponse.c_str());
}

float Application::Neuralyzer::AgentRemote::getContextCapacityUsage() const
{
    return mEngine.getContextUsage();
}

Application::Neuralyzer::ModelInfo Application::Neuralyzer::AgentRemote::getModelInfo() const
{
    return mEngine.getModelInfo();
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

// Runs a conversation with a model on a remote server to check the streaming, the tool calls and the abort.
// The test is only enabled if the environment variable PARTIELS_NEURALYZER_TEST_SERVER is set with the URL of an
// OpenAI-compatible server (e.g. llama-server). The model can be set with the environment variable
// PARTIELS_NEURALYZER_TEST_SERVER_MODEL (the first model of the server by default). The media are only tested if the
// model supports images.
class AgentRemoteTest
: public juce::UnitTest
{
public:
    AgentRemoteTest()
    : juce::UnitTest("Neuralyzer Agent Remote", "Application")
    {
    }

    ~AgentRemoteTest() override = default;

    void runTest() override
    {
        auto const serverUrl = juce::SystemStats::getEnvironmentVariable("PARTIELS_NEURALYZER_TEST_SERVER", {});
        if(serverUrl.isEmpty())
        {
            return;
        }

        beginTest("Model List");
        Application::Neuralyzer::ModelInfo info;
        info.serverUrl = juce::URL(serverUrl);
        info.modelId = juce::SystemStats::getEnvironmentVariable("PARTIELS_NEURALYZER_TEST_SERVER_MODEL", {});
        auto const models = Application::Neuralyzer::AgentRemote::getAvailableModels(info.serverUrl);
        expect(!models.empty(), "No model available on the server");
        if(models.empty())
        {
            return;
        }
        if(info.modelId.isEmpty())
        {
            info.modelId = models.cbegin()->first;
        }
        auto const modelIt = std::find_if(models.cbegin(), models.cend(), [&](auto const& model)
                                          {
                                              return model.first == info.modelId;
                                          });
        expect(modelIt != models.cend(), "The model is not available on the server");
        auto const supportsImage = modelIt != models.cend() && modelIt->second;
        logMessage("Model " + info.modelId + " (images " + juce::String(supportsImage ? "supported" : "not supported") + ")");
        // The image support is compared with the modalities reported by llama.cpp servers
        auto const hasVision = [&]() -> std::optional<bool>
        {
            try
            {
                auto const props = nlohmann::json::parse(juce::URL(serverUrl).getChildURL("props").readEntireTextStream().toStdString());
                return props.at("modalities").value("vision", false);
            }
            catch(...)
            {
                return std::nullopt;
            }
        }();
        expect(!hasVision.has_value() || hasVision.value() == supportsImage, "The image support does not match the modalities of the server");

        beginTest("Initialization");
        Application::Neuralyzer::Mcp::Dispatcher dispatcher(Application::Instance::get());
        Application::Neuralyzer::Rag::Engine engine;
        Application::Neuralyzer::AgentRemote agent(dispatcher, engine);
        auto const initResult = agent.initializeModel(info);
        expect(initResult.wasOk(), initResult.getErrorMessage());
        if(initResult.failed())
        {
            return;
        }
        auto const contextSize = agent.getModelInfo().contextSize.value_or(0);
        logMessage("Model " + info.modelId + " (context size " + juce::String(contextSize) + ")");
        expect(agent.startSession().wasOk());

        auto const ask = [&](juce::String const& prompt, juce::String const& expected, bool useThinkingMode = false)
        {
            auto const start = juce::Time::getMillisecondCounterHiRes();
            auto const result = agent.sendQuery(prompt + " Answer briefly and do not use tools.", useThinkingMode);
            auto const duration = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
            auto const history = agent.getHistory();
            auto const answer = history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str());
            logMessage(prompt + " -> " + answer + " (" + juce::String(duration, 2) + "s, context usage " + juce::String(agent.getContextCapacityUsage(), 3) + ")");
            expect(result.wasOk(), result.getErrorMessage());
            expect(expected.isEmpty() || answer.containsIgnoreCase(expected), answer);
        };

        auto const hasToolMessage = [&](juce::String const& text)
        {
            auto const history = agent.getHistory();
            return std::any_of(history.cbegin(), history.cend(), [&](auto const& message)
                               {
                                   return message.role == "tool" && message.tool_name == "read_files" && juce::String::fromUTF8(message.content.c_str()).contains(text);
                               });
        };

        beginTest("Multi-turn");
        // The model may refuse to answer about personal information (the instructions restrict the agent to Partiels),
        // so the questions are about the document
        ask("The Partiels document I am working on is named Zorglub and it contains 47 tracks.", {});
        ask("What is the name of the Partiels document I am working on?", "Zorglub");
        ask("How many tracks would the document contain if I added 3 tracks?", "50");
        expect(contextSize <= 0 || agent.getContextCapacityUsage() > 0.0f, "The context usage is not reported");

        beginTest("Thinking");
        {
            ask("How many tracks would the document contain if I removed 7 tracks?", "40", true);
            auto const history = agent.getHistory();
            expect(!history.empty() && !history.back().reasoning_content.empty(), "The reasoning is not reported");
        }

        beginTest("Session");
        {
            auto const sessionFile = juce::File::createTempFile("ptldoc");
            expect(agent.saveSession(sessionFile).wasOk());
            expect(agent.loadSession(sessionFile).wasOk());
            ask("How many tracks does the document contain?", "47");
            sessionFile.deleteFile();
        }

        if(supportsImage)
        {
            beginTest("Media");
            juce::TemporaryFile imageFile(".png");
            expect(imageFile.getFile().replaceWithData(AnlResourceData::Ircamlogo_png, static_cast<size_t>(AnlResourceData::Ircamlogo_pngSize)));
            auto const result = agent.sendQuery("Call the tool read_files once to read the image file " + imageFile.getFile().getFullPathName() + ", then tell me which institute is named in the image.", false);
            auto const history = agent.getHistory();
            auto const answer = history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str());
            logMessage("Media -> " + answer);
            expect(result.wasOk(), result.getErrorMessage());
            // The media are extracted from the content of the tool message to its content parts
            auto const hasImage = std::any_of(history.cbegin(), history.cend(), [](auto const& message)
                                              {
                                                  return message.role == "tool" && std::any_of(message.content_parts.cbegin(), message.content_parts.cend(), [](auto const& part)
                                                                                               {
                                                                                                   return part.type == "image_url";
                                                                                               });
                                              });
            expect(hasImage, "The image has not been attached");
            expect(answer.containsIgnoreCase("IRCAM"), answer);
        }

        beginTest("Tool Call");
        {
            juce::TemporaryFile textFile(".txt");
            expect(textFile.getFile().replaceWithText("The secret code of the studio is FANTASIO-1946."));
            auto const result = agent.sendQuery("Call the tool read_files once to read the file " + textFile.getFile().getFullPathName() + ", then tell me the secret code written in the file.", false);
            auto const history = agent.getHistory();
            auto const answer = history.empty() ? juce::String{} : juce::String::fromUTF8(history.back().content.c_str());
            logMessage("Tool call -> " + answer);
            expect(result.wasOk(), result.getErrorMessage());
            expect(hasToolMessage("FANTASIO-1946"), "The file has not been read");
            expect(answer.contains("FANTASIO-1946"), answer);
        }

        beginTest("Relative Path");
        {
            auto const result = agent.sendQuery("Call the tool read_files once with the relative path 'notes/studio.txt' exactly as written, then tell me the result of the tool.", false);
            expect(result.wasOk(), result.getErrorMessage());
            expect(hasToolMessage("The path is not absolute."), "The relative path has not been rejected");
        }

        // Aborts a query from another thread when a condition is met and checks that the query stops quickly and that
        // the history is restored
        auto const abortQuery = [&](juce::String const& prompt, std::function<bool()> const& shouldAbort)
        {
            auto const numMessages = agent.getHistory().size();
            std::atomic<bool> running{true};
            std::atomic<double> abortTime{0.0};
            std::thread stopper([&]()
                                {
                                    while(running.load())
                                    {
                                        if(shouldAbort())
                                        {
                                            abortTime.store(juce::Time::getMillisecondCounterHiRes());
                                            agent.setShouldQuit(true);
                                            return;
                                        }
                                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                    }
                                });
            auto const result = agent.sendQuery(prompt, false);
            auto const endTime = juce::Time::getMillisecondCounterHiRes();
            running.store(false);
            stopper.join();
            agent.setShouldQuit(false);
            expect(result.failed(), "The query has not been aborted");
            expectEquals(agent.getHistory().size(), numMessages);
            if(abortTime.load() > 0.0)
            {
                auto const latency = endTime - abortTime.load();
                logMessage("Abort latency: " + juce::String(latency, 1) + "ms (" + result.getErrorMessage() + ")");
                expect(latency < 2000.0, juce::String(latency));
            }
        };

        beginTest("Abort During Generation");
        {
            // The response must be long enough to abort the query during the generation (the temporary response of the
            // previous query is only cleared when the query starts)
            auto const prompt = juce::String("Write a detailed paragraph of about 200 words that describes the studio and that includes its secret code. Do not use tools.");
            auto const previousResponse = agent.getTemporaryResponse();
            abortQuery(prompt, [&]()
                       {
                           auto const response = agent.getTemporaryResponse();
                           return response != previousResponse && response.length() > 300;
                       });
            ask("What was the secret code of the studio?", "FANTASIO-1946");
        }

        beginTest("Abort During Prompt Evaluation");
        {
            // The long prompt keeps the server busy before the first token
            juce::String prompt = "Here is the inventory of my studio:";
            for(auto i = 1; i <= 1000; ++i)
            {
                prompt << " item " << i << " is a box of " << (i * 7) % 100 << " cables;";
            }
            prompt << " Just acknowledge.";
            auto const start = juce::Time::getMillisecondCounterHiRes();
            abortQuery(prompt, [&]()
                       {
                           return juce::Time::getMillisecondCounterHiRes() - start > 500.0;
                       });
            ask("What is the name of the Partiels document I am working on?", "Zorglub");
        }
    }
};

static AgentRemoteTest agentRemoteTest;

ANALYSE_FILE_END
