#include "AnlApplicationNeuralyzerGuard.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

namespace
{
    // Guard generation temperature: 0.0 keeps deterministic greedy decoding.
    constexpr float kGuardSamplingTemperature = 0.0f;
} // namespace

juce::File Application::Neuralyzer::Guard::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Guard");
}

juce::URL Application::Neuralyzer::Guard::getModelUrl()
{
    return juce::URL(PARTIELS_NEURALYZER_GUARD_MODEL_URL);
}

juce::File Application::Neuralyzer::Guard::getModelFile()
{
    return getDefaultModelDirectory().getChildFile(getModelUrl().getFileName());
}

void Application::Neuralyzer::Guard::downloadModelIfNecessary()
{
    if(getModelFile().existsAsFile())
    {
        return;
    }

    auto const callback = [](Downloader::Process const& process)
    {
        if(process.getResult().failed() || !process.getOutputFile().existsAsFile())
        {
            return;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("Guard Model Downloaded"))
                                 .withMessage(juce::translate("The Guard model has been successfully downloaded. You must restart the application to enable safety filtering."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    };

    Instance::get().getNeuralyzerDownloaderManager().start(getDefaultModelDirectory(), getModelUrl(), callback);
}

juce::Result Application::Neuralyzer::Guard::initializeModel(juce::File modelFile)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mContext.reset();
    mModel.reset();
    mChatTemplates.reset();

    if(modelFile == juce::File{} || !modelFile.existsAsFile())
    {
        return juce::Result::fail(juce::translate("Guard model file does not exist: FLNAME").replace("FLNAME", modelFile.getFullPathName()));
    }

    auto modelParams = llama_model_default_params();
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;
    mModel.reset(llama_model_load_from_file(modelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mModel == nullptr)
    {
        return juce::Result::fail(juce::translate("Failed to load Guard model: FLNAME").replace("FLNAME", modelFile.getFullPathName()));
    }

    if(!llama_model_has_encoder(mModel.get()) && !llama_model_has_decoder(mModel.get()))
    {
        mModel.reset();
        return juce::Result::fail(juce::translate("Guard model does not support encoder nor decoder."));
    }

    auto contextParams = llama_context_default_params();
    contextParams.n_ctx = 4096;
    contextParams.n_batch = 512;
    contextParams.n_seq_max = 1;
    contextParams.no_perf = true;

    mContext.reset(llama_init_from_model(mModel.get(), contextParams));
    if(mContext == nullptr)
    {
        mModel.reset();
        return juce::Result::fail(juce::translate("Failed to initialize Guard model context."));
    }

    try
    {
        mChatTemplates = common_chat_templates_init(mModel.get(), std::string{});
    }
    catch(...)
    {
        mContext.reset();
        mModel.reset();
        return juce::Result::fail(juce::translate("Failed to initialize Guard chat templates."));
    }
    if(mChatTemplates == nullptr)
    {
        mContext.reset();
        mModel.reset();
        return juce::Result::fail(juce::translate("Failed to initialize Guard chat templates."));
    }
    return juce::Result::ok();
}

void Application::Neuralyzer::Guard::reset()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mChatTemplates.reset();
    mContext.reset();
    mModel.reset();
}

bool Application::Neuralyzer::Guard::isReady() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mModel != nullptr && mContext != nullptr && mChatTemplates != nullptr;
}

juce::Result Application::Neuralyzer::Guard::checkText(juce::String const& text)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if(mModel == nullptr || mContext == nullptr || mChatTemplates == nullptr)
    {
        return juce::Result::ok();
    }

    common_chat_templates_inputs chatInputs;
    chatInputs.use_jinja = true;
    chatInputs.add_generation_prompt = true;
    chatInputs.tool_choice = COMMON_CHAT_TOOL_CHOICE_NONE;
    auto const* modelVocab = llama_model_get_vocab(mModel.get());
    chatInputs.add_bos = llama_vocab_get_add_bos(modelVocab);
    chatInputs.add_eos = llama_vocab_get_add_eos(modelVocab);

    common_chat_msg userMsg;
    userMsg.role = "user";
    userMsg.content = text.trim().toStdString();
    chatInputs.messages.push_back(userMsg);

    auto const originalPrompt = [&, this]() -> juce::String
    {
        try
        {
            return juce::String(common_chat_templates_apply(mChatTemplates.get(), chatInputs).prompt);
        }
        catch(...)
        {
            return {};
        }
    }();
    static auto constexpr beginCategoryTag = "<BEGIN UNSAFE CONTENT CATEGORIES>";
    static auto constexpr endCategoryTag = "<END UNSAFE CONTENT CATEGORIES>";
    auto const startSection = originalPrompt.upToFirstOccurrenceOf(beginCategoryTag, true, false);
    auto const categorySection = originalPrompt.fromFirstOccurrenceOf(beginCategoryTag, false, false).upToFirstOccurrenceOf(endCategoryTag, false, false);
    auto const endSection = originalPrompt.fromFirstOccurrenceOf(endCategoryTag, true, false);

    // Create a map of categories so we can retrieve the category description later
    auto categoriesLines = juce::StringArray::fromLines(categorySection);
    juce::StringPairArray categoryMap;
    for(auto category : categoriesLines)
    {
        auto const key = category.upToFirstOccurrenceOf(":", false, false);
        auto const value = category.fromFirstOccurrenceOf(":", false, false).trim();
        categoryMap.set(key, value);
    }

    // Remove the catagories S1, S2, S5, S6, S7 and S8 corresponding to
    // S1: Violent Crimes.
    // S2: Non-Violent Crimes.
    // S5: Defamation.
    // S6: Specialized Advice.
    // S7: Privacy.
    // S8: Intellectual Property.
    for(auto index : {"S1", "S2", "S5", "S6", "S7", "S8"})
    {
        for(auto lineIndex = 0; lineIndex < categoriesLines.size(); ++lineIndex)
        {
            if(categoriesLines[lineIndex].trim().startsWith(index))
            {
                categoriesLines.remove(lineIndex);
                break;
            }
        }
    }

    auto const prompt = startSection + categoriesLines.joinIntoString("\n") + endSection;
    auto const* utf8Text = prompt.toRawUTF8();
    MiscDebug("Guard", prompt);
    auto const utf8Size = static_cast<int32_t>(prompt.getNumBytesAsUTF8());
    auto const* vocab = llama_model_get_vocab(mModel.get());
    auto numTokens = llama_tokenize(vocab, utf8Text, utf8Size, nullptr, 0, true, true);
    if(numTokens == 0 || numTokens == std::numeric_limits<int32_t>::min())
    {
        return juce::Result::ok();
    }
    if(numTokens < 0)
    {
        numTokens = -numTokens;
    }

    std::vector<llama_token> tokens(static_cast<size_t>(numTokens));
    auto const tokenizedCount = llama_tokenize(vocab, utf8Text, utf8Size, tokens.data(), numTokens, true, true);
    if(tokenizedCount <= 0)
    {
        return juce::Result::ok();
    }

    if(auto* memory = llama_get_memory(mContext.get()))
    {
        llama_memory_clear(memory, true);
    }

    auto const batchSize = static_cast<size_t>(llama_n_batch(mContext.get()));
    if(batchSize == 0)
    {
        return juce::Result::ok();
    }

    size_t position = 0;
    int32_t lastBatchTokenCount = 0;
    while(position < static_cast<size_t>(tokenizedCount))
    {
        auto const chunkSize = std::min(batchSize, static_cast<size_t>(tokenizedCount) - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(chunkSize));
        auto const status = llama_decode(mContext.get(), batch);
        if(status != 0)
        {
            return juce::Result::ok();
        }
        lastBatchTokenCount = static_cast<int32_t>(chunkSize);
        position += chunkSize;
    }

    if(llama_get_logits_ith(mContext.get(), lastBatchTokenCount - 1) == nullptr)
    {
        return juce::Result::ok();
    }

    common_params_sampling samplingParams;
    samplingParams.temp = kGuardSamplingTemperature;
    common_sampler_ptr sampler(common_sampler_init(mModel.get(), samplingParams));
    if(sampler == nullptr)
    {
        return juce::Result::ok();
    }

    std::string generated;
    auto* context = mContext.get();
    while(true)
    {
        auto newTokenId = common_sampler_sample(sampler.get(), context, -1);
        common_sampler_accept(sampler.get(), newTokenId, true);

        if(llama_vocab_is_eog(vocab, newTokenId))
        {
            break;
        }

        generated += common_token_to_piece(vocab, newTokenId, true);

        auto tokenBatch = llama_batch_get_one(&newTokenId, 1);
        if(llama_decode(context, tokenBatch) != 0)
        {
            break;
        }

        if(llama_get_logits_ith(context, 0) == nullptr)
        {
            break;
        }
    }

    auto lines = juce::StringArray::fromLines(juce::String(generated).trim().toLowerCase());
    lines.removeEmptyStrings();
    if(!lines.isEmpty() && lines[0].toLowerCase().contains("unsafe"))
    {
        auto const categoryKey = lines.size() < 2 ? juce::String{} : lines[1];
        auto const category = categoryMap.getValue(categoryKey, "Unknown category");
        return juce::Result::fail(juce::translate("Unsafe content detected: CATEGORY.").replace("CATEGORY", category));
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
