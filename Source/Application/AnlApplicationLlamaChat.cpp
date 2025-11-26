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
    // MiscDebug("Application::Llama::Chat", "Decoding batch: numCtxUsed = " + juce::String(numCtxUsed) + ", batch.n_tokens = " + juce::String(batch.n_tokens) + ", numCtx = " + juce::String(numCtx));
    MiscWeakAssert(static_cast<uint32_t>(numCtxUsed + batch.n_tokens) <= numCtx);
    if(static_cast<uint32_t>(numCtxUsed + batch.n_tokens) > numCtx)
    {
        MiscDebug("Application::Llama::Chat", "Context size exceeded");
        return juce::Result::fail(juce::translate("Context size exceeded."));
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

Application::Llama::Chat::Chat(std::atomic<bool> const& shouldQuit)
: mShouldQuit(shouldQuit)
{
    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    // Load dynamic backends
    ggml_backend_load_all();
    llama_backend_init();
}

Application::Llama::Chat::~Chat()
{
    llama_log_set(nullptr, nullptr);
    llama_backend_free();
}

juce::Result Application::Llama::Chat::initialize(juce::File model)
{
    static int32_t constexpr numGpuLayers = 30;
    static uint32_t constexpr contextSize = 65536 * 2;
    static auto constexpr temperature = 0.1f;
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
    ctx_params.n_ctx = 0; // Use the model's default context size
    ctx_params.n_batch = contextSize;
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
    mMessageData.push_back(content);
    mMessages.push_back({chatRoles[magic_enum::enum_index(role).value_or(0)], mMessageData.back().c_str()});

    auto const* chatTemplate = llama_model_chat_template(mModel.get(), nullptr);
    auto const startAssistant = role == Role::user;
    auto const applyChatTemplate = [&, this]()
    {
        auto const* messagePtr = mMessages.data();
        auto const messageSize = mMessages.size();
        auto* formattedPtr = startAssistant ? mFormattedMessage.data() : nullptr;
        auto const formattedSize = startAssistant ? static_cast<int32_t>(mFormattedMessage.size()) : 0;
        return static_cast<long>(llama_chat_apply_template(chatTemplate, messagePtr, messageSize, startAssistant, formattedPtr, formattedSize));
    };
    auto newLength = applyChatTemplate();
    if(newLength > static_cast<long>(mFormattedMessage.size()))
    {
        mFormattedMessage.resize(static_cast<size_t>(newLength));
        newLength = applyChatTemplate();
    }
    if(newLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to apply the chat template");
    }
    return newLength;
}

juce::Result Application::Llama::Chat::addContext(juce::String const& content)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    auto const newLength = addMessage(Role::system, content.toStdString());
    MiscWeakAssert(newLength >= 0);
    if(newLength < 0)
    {
        return juce::Result::fail(juce::translate("Failed to apply the chat template."));
    }

    MiscDebug("Application::Llama::Chat", juce::String("Message ") + mMessages.back().role + ": " + mMessages.back().content);
    
    // remove previous messages to obtain the prompt to generate the response
    MiscWeakAssert(mPrevMessageLength >= 0 && static_cast<size_t>(mPrevMessageLength) < mFormattedMessage.size());
    std::string prompt(std::next(mFormattedMessage.cbegin(), mPrevMessageLength), std::next(mFormattedMessage.cbegin(), newLength));

    auto tokenResult = tokenize(mContext.get(), prompt.c_str(), prompt.size(), false);
    if(std::get<0_z>(tokenResult).failed())
    {
        return std::get<0_z>(tokenResult);
    }

    auto const tokenData = std::get<1_z>(tokenResult).data();
    auto const tokenSize = std::get<1_z>(tokenResult).size();
    auto const numCtx = llama_n_ctx(mContext.get());
    auto position = 0_z;
    while(position < tokenSize)
    {
        auto const size = std::min(static_cast<size_t>(numCtx), tokenSize - position);
        auto batch = llama_batch_get_one(tokenData + position, static_cast<int32_t>(size));
#if JUCE_DEBUG
        auto const vocabSize = llama_vocab_n_tokens(llama_model_get_vocab(llama_get_model(mContext.get())));
        for(int i = 0; i < batch.n_tokens; i++)
        {
            MiscWeakAssert(batch.token[i] >= 0 && batch.token[i] < vocabSize);
            if(batch.token[i] < 0 || batch.token[i] > vocabSize)
            {
                MiscDebug("Application::Llama::Chat", "Invalid token id: " + juce::String(batch.token[i]));
            }
        }
#endif
        auto const result = decode(mContext.get(), batch);
        if(result.failed())
        {
            return result;
        }
        position += size;
    }
    
    mPrevMessageLength = newLength;
    return juce::Result::ok();
}

juce::Result Application::Llama::Chat::loadState(juce::File state)
{
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return juce::Result::fail(juce::translate("The model is not initialized."));
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
    
    // Restore the loaded tokens
    auto position = 0_z;
    while(position < nloaded)
    {
        auto const size = std::min(static_cast<size_t>(numCtx), nloaded - position);
        auto batch = llama_batch_get_one(array.data() + position, static_cast<int32_t>(size));
#if JUCE_DEBUG
        auto const vocabSize = llama_vocab_n_tokens(llama_model_get_vocab(llama_get_model(mContext.get())));
        for(int i = 0; i < batch.n_tokens; i++)
        {    
            MiscWeakAssert(batch.token[i] >= 0 && batch.token[i] < vocabSize);
            if(batch.token[i] < 0 || batch.token[i] > vocabSize)
            {
                MiscDebug("Application::Llama::Chat", "Invalid token id: " + juce::String(batch.token[i]));
            }
        }
#endif
        auto const result = decode(mContext.get(), batch);
        if(result.failed())
        {
            return result;
        }
        position += size;
    }
    
    return juce::Result::ok();
}

std::tuple<juce::Result, juce::String, juce::String> Application::Llama::Chat::generate(Role const role, juce::String const& userMessage)
{
    MiscWeakAssert(role != Role::assistant);
    if(mContext == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), juce::String{}, juce::String{});
    }

    // Set log callback to suppress unnecessary output
    llama_log_set(llama_log_callback, nullptr);

    auto newLength = addMessage(role, userMessage.toStdString());
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
        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
    }

    std::string response;
    // Prepare a batch for the prompt
    auto batch = llama_batch_get_one(std::get<1_z>(tokenResult).data(), static_cast<int32_t>(std::get<1_z>(tokenResult).size()));
    while(true)
    {
        if(mShouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
        auto result = decode(mContext.get(), batch);
        if(result.failed())
        {
            break;
        }

        if(mShouldQuit.load())
        {
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
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
                startIt = include ? startIt : startIt + startLength;
                endIt = include ? endIt + endLength : endIt;
                return response.substr(startIt, endIt - startIt);
            }
        }
        return {};
    };
    auto const message = split("<response>", "</response>", false);
    auto const document = split("<document>", "</document>", true);
    return std::make_tuple(juce::Result::ok(), message, document);
}

ANALYSE_FILE_END
