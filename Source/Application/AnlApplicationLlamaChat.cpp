#include "AnlApplicationLlamaChat.h"

ANALYSE_FILE_BEGIN

Application::Llama::Chat::Chat()
{
    // Set log callback to suppress unnecessary output
    llama_log_set([](ggml_log_level level, char const* text, void*)
                  {
                      if(level >= GGML_LOG_LEVEL_ERROR)
                      {
                          fprintf(stderr, "%s", text);
                      }
                  },
                  nullptr);

    // Load dynamic backends
    ggml_backend_load_all();
}

Application::Llama::Chat::~Chat()
{
    llama_log_set(nullptr, nullptr);
}

juce::Result Application::Llama::Chat::initialize(juce::File model, juce::File state)
{
    static int32_t constexpr numGpuLayers = 30;
    static uint32_t constexpr contextSize = 8192;
    static auto constexpr temperature = 0.3f;
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

    // Initialize the model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = numGpuLayers;

    mModel = ModelPtr(llama_model_load_from_file(model.getFullPathName().toRawUTF8(), model_params), &llama_model_free);
    if(mModel == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Failed to load model from: " + model.getFullPathName());
        return juce::Result::fail(juce::translate("Failed to load model from: FLNAME").replace("FLNAME", model.getFullPathName()));
    }

    mVocab = llama_model_get_vocab(mModel.get());

    // Initialize the context
    auto ctx_params = llama_context_default_params();
    ctx_params.n_ctx = contextSize;
    ctx_params.n_batch = contextSize;
    ctx_params.abort_callback = [](void* data)
    {
        auto const* shouldQuit = reinterpret_cast<Chat*>(data)->mShouldQuit;
        return shouldQuit != nullptr && shouldQuit->load();
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

    if(state.existsAsFile())
    {
        size_t nloaded = 0;
        llama_state_load_file(mContext.get(), state.getFullPathName().toRawUTF8(), nullptr, 0, &nloaded);
        MiscDebug("Application::Llama::Chat", juce::String("Loaded ") + juce::String(nloaded) + juce::String(" tokens from state file: ") + state.getFullPathName());
    }

    return juce::Result::ok();
}

std::tuple<juce::Result, juce::String, juce::String> Application::Llama::Chat::generate(Role const role, juce::String const& userMessage, std::atomic<bool> const& shouldQuit)
{
    MiscWeakAssert(role != Role::assistant);
    if(mContext == nullptr || mVocab == nullptr || mSampler == nullptr)
    {
        MiscDebug("Application::Llama::Chat", "Not initialized");
        return std::make_tuple(juce::Result::fail(juce::translate("The model is not initialized.")), juce::String{}, juce::String{});
    }

    mShouldQuit = &shouldQuit;
    auto const* chatTemplate = llama_model_chat_template(mModel.get(), nullptr);
    auto const addMessage = [this](Role const erole, std::string const& content)
    {
        static const char* const chatRoles[] = {"assistant", "system", "user"};
        mMessageData.push_back(content);
        mMessages.push_back({chatRoles[magic_enum::enum_index(erole).value_or(0)], mMessageData.back().c_str()});
        MiscDebug("Application::Llama::Chat", juce::String("Message ") + mMessages.back().role + ": " + mMessages.back().content);
    };
    auto const applyChatTemplate = [&, this](bool add)
    {
        auto const* messagePtr = mMessages.data();
        auto const messageSize = mMessages.size();
        auto* formattedPtr = add ? mFormattedMessage.data() : nullptr;
        auto const formattedSize = add ? static_cast<int32_t>(mFormattedMessage.size()) : 0;
        return static_cast<long>(llama_chat_apply_template(chatTemplate, messagePtr, messageSize, add, formattedPtr, formattedSize));
    };

    addMessage(role, userMessage.toStdString());
    auto newLength = applyChatTemplate(true);
    if(newLength > static_cast<long>(mFormattedMessage.size()))
    {
        mFormattedMessage.resize(static_cast<size_t>(newLength));
        newLength = applyChatTemplate(true);
    }
    if(newLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to apply the chat template");
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to apply the chat template.")), juce::String{}, juce::String{});
    }

    // remove previous messages to obtain the prompt to generate the response
    MiscWeakAssert(mPrevMessageLength >= 0 && static_cast<size_t>(mPrevMessageLength) < mFormattedMessage.size());
    std::string prompt(std::next(mFormattedMessage.cbegin(), mPrevMessageLength), std::next(mFormattedMessage.cbegin(), newLength));

    // Check if this is the first generation in the sequence
    auto const is_first = llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) == -1;

    // Tokenize the prompt
    auto const n_prompt_tokens = -llama_tokenize(mVocab, prompt.c_str(), static_cast<int32_t>(prompt.size()), nullptr, 0, is_first, true);
    MiscWeakAssert(n_prompt_tokens >= 0);
    if(n_prompt_tokens < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to tokenize the prompt");
        mShouldQuit = nullptr;
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to tokenize the prompt.")), juce::String{}, juce::String{});
    }
    std::vector<llama_token> prompt_tokens(static_cast<uint32_t>(n_prompt_tokens));

    if(shouldQuit.load())
    {
        mShouldQuit = nullptr;
        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
    }

    if(llama_tokenize(mVocab, prompt.c_str(), static_cast<int32_t>(prompt.size()), prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()), is_first, true) < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to tokenize the prompt");
        mShouldQuit = nullptr;
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to tokenize the prompt.")), juce::String{}, juce::String{});
    }
    if(shouldQuit.load())
    {
        mShouldQuit = nullptr;
        return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
    }

    std::string response;
    // Prepare a batch for the prompt
    auto batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()));
    while(true)
    {
        if(shouldQuit.load())
        {
            mShouldQuit = nullptr;
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }
        // Check if we have enough space in the context
        auto const n_ctx = llama_n_ctx(mContext.get());
        auto const n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(mContext.get()), 0) + 1;
        MiscWeakAssert(static_cast<uint32_t>(n_ctx_used + batch.n_tokens) <= n_ctx);
        if(static_cast<uint32_t>(n_ctx_used + batch.n_tokens) > n_ctx)
        {
            MiscDebug("Application::Llama::Chat", "Context size exceeded");
            break;
        }

        // Decode the batch
        auto const ret = llama_decode(mContext.get(), batch);
        MiscWeakAssert(ret == 0);
        if(ret != 0)
        {
            MiscDebug("Application::Llama::Chat", "Failed to decode, ret = " + juce::String(ret));
            break;
        }
        if(shouldQuit.load())
        {
            mShouldQuit = nullptr;
            return std::make_tuple(juce::Result::ok(), juce::String{}, juce::String{});
        }

        // Sample the next token
        auto new_token_id = llama_sampler_sample(mSampler.get(), mContext.get(), -1);

        // Check if it's an end-of-generation token
        if(llama_vocab_is_eog(mVocab, new_token_id))
        {
            break;
        }

        auto const piece = [&, this]()
        {
            // Convert the token to a string and add it to the response
            char buf[1024];
            auto const n = llama_token_to_piece(mVocab, new_token_id, buf, sizeof(buf), 0, true);
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

    addMessage(Role::assistant, response);
    mPrevMessageLength = applyChatTemplate(false);
    MiscWeakAssert(mPrevMessageLength >= 0);
    if(mPrevMessageLength < 0)
    {
        MiscDebug("Application::Llama::Chat", "Failed to apply the chat template");
        mShouldQuit = nullptr;
        return std::make_tuple(juce::Result::fail(juce::translate("Failed to apply the chat template.")), juce::String{}, juce::String{});
    }
    mShouldQuit = nullptr;

    // Split the text of the response in the form <response>text</response><document>text</document> into two strings
    auto const split = [&](const char* start, const char* end) -> std::string
    {
        auto startIt = response.find(start);
        if(startIt != std::string::npos)
        {
            startIt += std::strlen(start);
            auto const endIt = response.find(end, startIt);
            if(endIt != std::string::npos)
            {
                return response.substr(startIt, endIt - startIt);
            }
        }
        return {};
    };
    auto const message = split("<response>", "</response>");
    auto const document = split("<document>", "</document>");
    return std::make_tuple(juce::Result::ok(), message, document);
}

ANALYSE_FILE_END
