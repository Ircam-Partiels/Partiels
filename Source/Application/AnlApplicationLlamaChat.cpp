#include "AnlApplicationLlamaChat.h"

ANALYSE_FILE_BEGIN

static void llama_log_callback(enum ggml_log_level level, const char* text, void*)
{
    if(level >= GGML_LOG_LEVEL_ERROR)
    {
        MiscDebug("Application::Llama::Chat", text);
    }
}

static std::tuple<juce::Result, std::vector<llama_token>> tokenize(llama_context const* context, char const* promptContent, size_t promptSize, bool specialCharacters)
{
    auto const* vocab = llama_model_get_vocab(llama_get_model(context));
    auto const isFirst = llama_memory_seq_pos_max(llama_get_memory(context), 0) == -1;
    auto const numPromptTokens = -llama_tokenize(vocab, promptContent, static_cast<int32_t>(promptSize), nullptr, 0, isFirst, specialCharacters);
    MiscWeakAssert(numPromptTokens >= 0);
    if(numPromptTokens < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to tokenize the prompt");
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to tokenize the prompt.")), std::vector<llama_token>{});
    }
    std::vector<llama_token> tokens(static_cast<uint32_t>(numPromptTokens));
    if(llama_tokenize(vocab, promptContent, static_cast<int32_t>(promptSize), tokens.data(), static_cast<int32_t>(tokens.size()), isFirst, specialCharacters) < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to tokenize the prompt");
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to tokenize the prompt.")), std::vector<llama_token>{});
    }
    return std::make_tuple(juce::Result::ok(), std::move(tokens));
}

static juce::Result decode(llama_context* context, llama_batch const& batch)
{
    // Check if we have enough space in the context
    auto const numCtx = llama_n_ctx(context);
    auto const numCtxUsed = llama_memory_seq_pos_max(llama_get_memory(context), 0) + 1;
    MiscWeakAssert(static_cast<uint32_t>(numCtxUsed + batch.n_tokens) <= numCtx);
    if(static_cast<uint32_t>(numCtxUsed + batch.n_tokens) > numCtx)
    {
        MiscDebug("Application::Llama::Chat", "Context size exceeded. numCtxUsed = " + juce::String(numCtxUsed) + ", batch.n_tokens = " + juce::String(batch.n_tokens) + ", numCtx = " + juce::String(numCtx));
        return juce::Result::fail(juce::translate("Context size exceeded. num. context used: NUMCTXUSED, num. batch tokens = BATCHTOKENS, num . context space = NUMCTX.").replace("NUMCTXUSED", juce::String(numCtxUsed)).replace("BATCHTOKENS", juce::String(batch.n_tokens)).replace("NUMCTX", juce::String(numCtx)));
    }

    // Decode the batch
    auto const ret = llama_decode(context, batch);
    MiscWeakAssert(ret == 0);
    if(ret != 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to decode, ret = " + juce::String(ret));
        return juce::Result::fail(juce::translate("Failed to decode ERRORCODE.").replace("ERRORCODE", juce::String(ret)));
    }
    return juce::Result::ok();
}

static juce::Result decodeTokensInBatches(llama_context* context, std::atomic<bool> const& shouldQuit, llama_token* tokenData, size_t tokenSize)
{
    auto const numCtx = llama_n_ctx(context);
    auto position = 0_z;
    while(position < tokenSize)
    {
        if(shouldQuit.load())
        {
            return juce::Result::fail(juce::translate("Operation aborted."));
        }

        auto const size = std::min(static_cast<size_t>(numCtx), tokenSize - position);
        auto batch = llama_batch_get_one(tokenData + position, static_cast<int32_t>(size));
#if JUCE_DEBUG
        auto const vocabSize = llama_vocab_n_tokens(llama_model_get_vocab(llama_get_model(context)));
        for(int i = 0; i < batch.n_tokens; i++)
        {
            MiscWeakAssert(batch.token[i] >= 0 && batch.token[i] < vocabSize);
            if(batch.token[i] < 0 || batch.token[i] > vocabSize)
            {
                MiscDebug("Application::Llama::Chat", "Invalid token id: " + juce::String(batch.token[i]));
            }
        }
#endif
        auto const result = decode(context, batch);
        if(result.failed())
        {
            return result;
        }
        position += size;
    }
    return juce::Result::ok();
}

Application::Llama::Chat::Chat(std::atomic<bool> const& shouldQuit)
: mShouldQuit(shouldQuit)
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []()
                   {
                       llama_log_set([](enum ggml_log_level, char const* text, void*)
                                     {
                                         MiscDebug("Application::Llama::Chat::Init", text);
                                     },
                                     nullptr);
                       // Load dynamic backends
                       ggml_backend_load_all();
                       // Initialize llama backend (required for Metal GPU on macOS)
                       llama_backend_init();
                   });
    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);
}

Application::Llama::Chat::~Chat()
{
    llama_log_set(nullptr, nullptr);
    llama_backend_free();
}

juce::Result Application::Llama::Chat::initialize(juce::File model)
{
    static int32_t constexpr numGpuLayers = 99;
    static uint32_t constexpr batchSize = 65536;
    static auto constexpr temperature = 0.2f;
    static auto constexpr minP = 0.05f;
    static uint32_t constexpr seed = LLAMA_DEFAULT_SEED;
    if(model == juce::File())
    {
        MiscDebug("Application::Llama::Chat", "The model file is not set.");
        return juce::Result::fail(juce::translate("The model file is not set."));
    }
    if(!model.existsAsFile())
    {
        MiscDebug("Application::Llama::Chat", "The model file does not exist: " + model.getFullPathName());
        return juce::Result::fail(juce::translate("The model file does not exist: FLNAME").replace("FLNAME", model.getFullPathName()));
    }
    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    // Initialize the model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = numGpuLayers;

    mModel = ModelPtr(llama_model_load_from_file(model.getFullPathName().toRawUTF8(), model_params), &llama_model_free);
    if(mModel == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Failed to load model from: " + model.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", model.getFullPathName()));
    }

    // Initialize the context
    auto ctx_params = llama_context_default_params();
    auto const modelMaxCtx = llama_model_n_ctx_train(mModel.get());
    ctx_params.n_ctx = std::min(static_cast<uint32_t>(32768), static_cast<uint32_t>(modelMaxCtx)); // Increased to 64K for better instruction retention
    ctx_params.n_batch = ctx_params.n_ctx;
    ctx_params.abort_callback = [](void* data)
    {
        return reinterpret_cast<Chat*>(data)->mShouldQuit.load();
    };
    ctx_params.abort_callback_data = static_cast<void*>(this);

    mContext = ContextPtr(llama_init_from_model(mModel.get(), ctx_params), &llama_free);
    if(mContext == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Failed to create llama context");
        return juce::Result::fail(juce::translate("Failed to create llama context"));
    }

    // Initialize the sampler
    mSampler = SamplerPtr(llama_sampler_chain_init(llama_sampler_chain_default_params()), &llama_sampler_free);
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_min_p(minP, 1));
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_dist(seed));
    // Add repetition penalties: last_n=64, repeat=1.15, freq=0.1, presence=0.1
    llama_sampler_chain_add(mSampler.get(), llama_sampler_init_penalties(64, 1.15f, 0.1f, 0.1f));

    mMessages.clear();
    mMessageData.clear();
    mFormattedMessage.clear();
    mFormattedMessage.resize(llama_n_ctx(mContext.get()));
    mPrevMessageLength = 0;
    MiscDebug("Application::Llama::Chat", "Successfully initialized model: " + model.getFullPathName());
    return juce::Result::ok();
}

long Application::Llama::Chat::addMessage(Role const role, std::string const& content)
{
    static const char* const chatRoles[] = {"assistant", "system", "user"};
    auto const roleIndex = [&]() -> size_t
    {
        switch(role)
        {
            case Role::assistant:
                return 0;
            case Role::system:
                return 1;
            case Role::user:
                return 2;
        }
        return 0;
    }();
    mMessageData.push_back(content);
    mMessages.push_back({chatRoles[roleIndex], mMessageData.back().c_str()});

    auto const* chatTemplate = llama_model_chat_template(mModel.get(), nullptr);
    auto const startAssistant = role == Role::user;

    // Get the required size first
    auto const* messagePtr = mMessages.data();
    auto const messageSize = mMessages.size();
    auto newLength = static_cast<long>(llama_chat_apply_template(chatTemplate, messagePtr, messageSize, startAssistant, nullptr, 0));

    if(newLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to apply the chat template");
        return newLength;
    }

    // Resize buffer if needed and format the messages
    if(newLength > static_cast<long>(mFormattedMessage.size()))
    {
        mFormattedMessage.resize(static_cast<size_t>(newLength));
    }
    newLength = static_cast<long>(llama_chat_apply_template(chatTemplate, messagePtr, messageSize, startAssistant, mFormattedMessage.data(), static_cast<int32_t>(mFormattedMessage.size())));
    if(newLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to format the chat template");
        return newLength;
    }

    return newLength;
}

juce::Result Application::Llama::Chat::loadState(juce::File state)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(!state.existsAsFile())
    {
        return juce::Result::fail(juce::translate("The state file does not exist: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    size_t nloaded = 0;
    auto const numCtx = llama_n_ctx(mContext.get());
    std::vector<llama_token> array(numCtx);
    if(!llama_state_load_file(mContext.get(), state.getFullPathName().toRawUTF8(), array.data(), array.size(), &nloaded))
    {
        MiscDebug("Application::Llama::Chat", "Failed to load state from file: " + state.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load state from file: FLNAME").replace("FLNAME", state.getFullPathName()));
    }
    if(nloaded == 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to load state from file (no tokens loaded): " + state.getFullPathName());
    }
    MiscDebug("Application::Llama::Chat", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + state.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Llama::Chat::saveState(juce::File state)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(state == juce::File())
    {
        return juce::Result::fail(juce::translate("The state file is not set."));
    }

    // Ensure directory exists
    auto const parentDir = state.getParentDirectory();
    if(!parentDir.exists())
    {
        if(!parentDir.createDirectory())
        {
            MiscDebug("Application::Llama::Chat", "Failed to create directory: " + parentDir.getFullPathName());
            return juce::Result::fail(juce::translate("Failed to create directory: FLNAME").replace("FLNAME", parentDir.getFullPathName()));
        }
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    // Save KV cache state. We don't persist prompt tokens here (nullptr, 0) since
    // the KV cache is sufficient for restoring the model state.
    bool const ok = llama_state_save_file(mContext.get(), state.getFullPathName().toRawUTF8(), nullptr, 0);
    if(!ok)
    {
        MiscDebug("Application::Llama::Chat", "Failed to save state to file: " + state.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to save state to file: FLNAME").replace("FLNAME", state.getFullPathName()));
    }

    MiscDebug("Application::Llama::Chat", juce::String("Saved KV cache to state file: ") + state.getFullPathName());
    return juce::Result::ok();
}

juce::Result Application::Llama::Chat::injectContext(juce::String const& content)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(content.isEmpty())
    {
        return juce::Result::fail(juce::translate("The context content is empty."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    // Tokenize the raw content WITHOUT chat template formatting
    // This is much faster for large documents that don't need a response
    auto const text = content.toStdString();
    auto tokenResult = tokenize(mContext.get(), text.c_str(), text.size(), false);
    if(std::get<0_z>(tokenResult).failed())
    {
        return std::get<0_z>(tokenResult);
    }

    // Decode tokens in batches to populate KV cache
    auto const tokenData = std::get<1_z>(tokenResult).data();
    auto const tokenSize = std::get<1_z>(tokenResult).size();
    auto const result = decodeTokensInBatches(mContext.get(), mShouldQuit, tokenData, tokenSize);
    if(result.failed())
    {
        return result;
    }

    MiscDebug("Application::Llama::Chat", juce::String("Injected ") + juce::String(tokenSize) + juce::String(" tokens of raw context"));
    MiscDebug("Application::Llama::Chat", content);
    return juce::Result::ok();
}

juce::Result Application::Llama::Chat::addSystemMessage(juce::String const& instruction)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    if(instruction.isEmpty())
    {
        MiscDebug("Application::Llama::Chat", "The instruction content is empty.");
        return juce::Result::fail(juce::translate("The instruction content is empty."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    // Add to message history with chat template formatting
    auto newLength = addMessage(Role::system, instruction.toStdString());
    MiscWeakAssert(newLength >= 0);
    if(newLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to apply the chat template.");
        return juce::Result::fail(juce::translate("Failed to apply the chat template."));
    }
    if(mShouldQuit.load())
    {
        return juce::Result::fail(juce::translate("Operation aborted."));
    }

    MiscDebug("Application::Llama::Chat", juce::String("System message: ") + instruction);

    // Extract just the new prompt portion (since last message)
    MiscWeakAssert(mPrevMessageLength >= 0 && static_cast<size_t>(mPrevMessageLength) < mFormattedMessage.size());
    std::string prompt(std::next(mFormattedMessage.cbegin(), mPrevMessageLength),
                       std::next(mFormattedMessage.cbegin(), newLength));

    // Tokenize the formatted system message
    auto tokenResult = tokenize(mContext.get(), prompt.c_str(), prompt.size(), false);
    if(std::get<0_z>(tokenResult).failed())
    {
        return std::get<0_z>(tokenResult);
    }

    // Decode tokens in batches to populate KV cache (no generation/sampling)
    auto const tokenData = std::get<1_z>(tokenResult).data();
    auto const tokenSize = std::get<1_z>(tokenResult).size();
    auto const result = decodeTokensInBatches(mContext.get(), mShouldQuit, tokenData, tokenSize);
    if(result.failed())
    {
        return result;
    }

    // Advance slicing cursor so subsequent generate() only includes the next user message
    mPrevMessageLength = newLength;

    MiscDebug("Application::Llama::Chat",
              juce::String("Added system message with ") + juce::String(tokenSize) + juce::String(" tokens"));
    return juce::Result::ok();
}

std::tuple<juce::Result, juce::String, juce::String> Application::Llama::Chat::sendUserQuery(juce::String const& userMessage)
{
    {
        std::unique_lock<std::mutex> lock(mTempResponseMutex);
        mTempResponse.clear();
    }

    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), juce::String{}, juce::String{});
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    auto newLength = addMessage(Role::user, userMessage.toStdString());
    MiscWeakAssert(newLength >= 0);
    if(newLength < 0)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to apply the chat template.")), juce::String{}, juce::String{});
    }
    MiscDebug("Application::Llama::Chat", juce::String("Message ") + mMessages.back().role + ": " + mMessages.back().content);

    // remove previous messages to obtain the prompt to generate the response
    MiscWeakAssert(mPrevMessageLength >= 0 && static_cast<size_t>(mPrevMessageLength) < mFormattedMessage.size());
    std::string prompt(std::next(mFormattedMessage.cbegin(), mPrevMessageLength), std::next(mFormattedMessage.cbegin(), newLength));

    auto tokenResult = tokenize(mContext.get(), prompt.c_str(), prompt.size(), true);
    if(std::get<0_z>(tokenResult).failed())
    {
        return std::make_tuple(std::get<0_z>(tokenResult), juce::String{}, juce::String{});
    }

    if(mShouldQuit.load())
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), juce::String{}, juce::String{});
    }

    std::string response;
    // Prepare a batch for the prompt
    auto result = juce::Result::ok();
    auto batch = llama_batch_get_one(std::get<1_z>(tokenResult).data(), static_cast<int32_t>(std::get<1_z>(tokenResult).size()));
    while(true)
    {
        if(mShouldQuit.load())
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), juce::String{}, juce::String{});
        }
        result = decode(mContext.get(), batch);
        if(result.failed())
        {
            break;
        }

        if(mShouldQuit.load())
        {
            return std::make_tuple(juce::Result::fail(juce::translate("Operation aborted.")), juce::String{}, juce::String{});
        }

        // Sample the next token
        auto new_token_id = llama_sampler_sample(mSampler.get(), mContext.get(), -1);
        auto const* vocab = llama_model_get_vocab(llama_get_model(mContext.get()));
        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(vocab, new_token_id))
        {
            break;
        }

        auto const piece = [&]()
        {
            // Convert the token to a string and add it to the response
            char buf[1024];
            auto const n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            MiscWeakAssert(n >= 0);
            if(n < 0)
            {
                MiscDebug("Application::Llama::Chat", "Failed to convert token to piece");
                return std::string{};
            }
            return std::string(buf, static_cast<size_t>(n));
        }();
        response += piece;
        // Count how many times the last responseWindowSize are present in the response
        static constexpr size_t responseWindowSize = 200; // only check for repetition if piece is at least this long
        static constexpr size_t maxRepetition = 10;       // number of times a string can repeat before aborting
        if(response.size() > responseWindowSize)
        {
            auto const startPos = response.size() - responseWindowSize;
            auto occurencePos = response.rfind(response.substr(startPos, responseWindowSize), startPos - 1);
            if(occurencePos != std::string::npos && occurencePos > 0)
            {
                auto const textSequence = response.substr(occurencePos, response.size() - occurencePos);
                occurencePos = response.rfind(textSequence, occurencePos - 1);
                auto textSequenceCount = 1_z;
                while(occurencePos != std::string::npos && occurencePos > 0)
                {
                    ++textSequenceCount;
                    if(textSequenceCount > maxRepetition)
                    {
                        MiscDebug("Application::Llama::Chat", "Aborting generation due to excessive repetition");
                        result = juce::Result::fail(juce::translate("Aborted generation due to repetition."));
                        break;
                    }
                    occurencePos = response.rfind(textSequence, occurencePos - 1);
                }
                if(result.failed())
                {
                    break;
                }
            }
        }

        {
            std::unique_lock<std::mutex> lock(mTempResponseMutex);
            mTempResponse = response;
        }

        // Prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

    mPrevMessageLength = addMessage(Role::assistant, response);
    MiscWeakAssert(mPrevMessageLength >= 0);
    if(mPrevMessageLength < 0)
    {
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to apply the chat template.")), juce::String{}, juce::String{});
    }

    MiscDebug("Application::Llama::Chat", juce::String("Message ") + mMessages.back().role + ": " + mMessages.back().content);
    // Split the text of the response in the form <response>text</response><document>text</document> into two strings
    auto const split = [&](const char* start, const char* end, bool include) -> std::string
    {
        auto startIt = response.find(start);
        if(startIt != std::string::npos)
        {
            auto const startLength = std::strlen(start);
            auto endIt = response.find(end, startIt + startLength);
            if(endIt != std::string::npos)
            {
                auto const endLength = std::strlen(end);
                auto const extractStart = include ? startIt : startIt + startLength;
                auto const extractEnd = include ? endIt + endLength : endIt;
                auto strResult = response.substr(extractStart, extractEnd - extractStart);
                auto const eraseStart = startIt;
                auto const eraseEnd = endIt + endLength;
                response.erase(eraseStart, eraseEnd - eraseStart);
                return strResult;
            }
        }
        return {};
    };

    auto const document = split("<document>", "</document>", true);
    auto message = split("<response>", "</response>", false);
    if(message.empty())
    {
        if(!response.empty())
        {
            message = response;
        }
        else if(!document.empty())
        {
            message = "The model modified the document.";
        }
        else
        {
            message = "The model did not generate a response.";
            if(result.wasOk())
            {
                result = juce::Result::fail(juce::translate("No response generated."));
            }
        }
    }
    MiscDebug("Application::Llama::Chat", result.getErrorMessage() + ", message: " + message + ", document: " + document);
    return std::make_tuple(result, message, document);
}

juce::String Application::Llama::Chat::getTemporaryResponse() const
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

void Application::Llama::Chat::clearTemporaryResponse()
{
    std::unique_lock<std::mutex> lock(mTempResponseMutex);
    mTempResponse.clear();
}

ANALYSE_FILE_END
