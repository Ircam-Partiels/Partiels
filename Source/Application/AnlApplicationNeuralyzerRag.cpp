#include "AnlApplicationNeuralyzerRag.h"
#include "AnlApplicationNeuralyzerModel.h"

#include <AnlNeuralyzerData.h>

ANALYSE_FILE_BEGIN

void Application::Neuralyzer::Rag::to_json(nlohmann::json& json, Application::Neuralyzer::Rag::Resource const& resource)
{
    json = nlohmann::json{{"id", resource.id.toStdString()},
                          {"content", resource.content.toStdString()},
                          {"section", resource.section.toStdString()},
                          {"document", resource.document.toStdString()}};
}

void Application::Neuralyzer::Rag::from_json(nlohmann::json const& json, Application::Neuralyzer::Rag::Resource& resource)
{
    resource.id = juce::String(json.value("id", std::string{})).trim();
    resource.content = juce::String(json.value("content", std::string{})).trim();
    resource.section = juce::String(json.value("section", std::string{})).trim();
    resource.document = juce::String(json.value("document", std::string{})).trim();
}

bool Application::Neuralyzer::Rag::Resource::operator==(Resource const& rhs) const noexcept
{
    return id == rhs.id &&
           content == rhs.content &&
           section == rhs.section &&
           document == rhs.document;
}

bool Application::Neuralyzer::Rag::Resource::operator<(Resource const& rhs) const
{
    if(id.compareNatural(rhs.id) != 0)
    {
        return id.compareNatural(rhs.id) < 0;
    }
    if(document.compareNatural(rhs.document) != 0)
    {
        return document.compareNatural(rhs.document) < 0;
    }
    if(section.compareNatural(rhs.section) != 0)
    {
        return section.compareNatural(rhs.section) < 0;
    }
    return content.compareNatural(rhs.content) < 0;
}

juce::Result Application::Neuralyzer::Rag::Engine::initializeModel(juce::File modelFile)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mModelFile = std::move(modelFile);

    mEmbeddingContext.reset();
    mEmbeddingModel.reset();
    if(mModelFile == juce::File())
    {
        return juce::Result::ok();
    }

    if(!mModelFile.existsAsFile())
    {
        return juce::Result::fail(juce::translate("RAG model file does not exist: FLNAME").replace("FLNAME", mModelFile.getFullPathName()));
    }

    auto modelParams = llama_model_default_params();
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;
    mEmbeddingModel.reset(llama_model_load_from_file(mModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mEmbeddingModel == nullptr)
    {
        return juce::Result::fail(juce::translate("Failed to load RAG model: FLNAME").replace("FLNAME", mModelFile.getFullPathName()));
    }

    auto const hasEncoder = llama_model_has_encoder(mEmbeddingModel.get());
    auto const hasDecoder = llama_model_has_decoder(mEmbeddingModel.get());
    if(!hasEncoder && !hasDecoder)
    {
        mEmbeddingModel.reset();
        return juce::Result::fail(juce::translate("RAG model does not support encoder nor decoder."));
    }

    auto contextParams = llama_context_default_params();
    contextParams.n_ctx = 2048;
    contextParams.n_batch = 2048;
    contextParams.n_ubatch = 2048;
    contextParams.n_seq_max = 1;
    contextParams.embeddings = true;
    contextParams.pooling_type = LLAMA_POOLING_TYPE_UNSPECIFIED;
    contextParams.no_perf = true;

    mEmbeddingContext.reset(llama_init_from_model(mEmbeddingModel.get(), contextParams));
    if(mEmbeddingContext == nullptr)
    {
        mEmbeddingModel.reset();
        return juce::Result::fail(juce::translate("Failed to initialize RAG model context."));
    }

    if(llama_model_n_embd_out(mEmbeddingModel.get()) <= 0)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        return juce::Result::fail(juce::translate("RAG model embedding size is invalid."));
    }

    mIndex.reset();
    mIndexedEntries.clear();
    return juce::Result::ok();
}

std::vector<float> Application::Neuralyzer::Rag::Engine::computeEmbedding(juce::String const& text)
{
    MiscWeakAssert(mEmbeddingModel != nullptr && mEmbeddingContext != nullptr);
    if(mEmbeddingModel == nullptr || mEmbeddingContext == nullptr)
    {
        return {};
    }
    if(text.isEmpty())
    {
        return {};
    }

    auto* model = mEmbeddingModel.get();

    auto const* utf8Text = text.toRawUTF8();
    auto const utf8Size = text.getNumBytesAsUTF8();
    auto const* vocab = llama_model_get_vocab(model);
    auto numTokens = llama_tokenize(vocab, utf8Text, static_cast<int32_t>(utf8Size), nullptr, 0, true, true);
    MiscWeakAssert(numTokens != 0 && numTokens != std::numeric_limits<int32_t>::min());
    if(numTokens == 0 || numTokens == std::numeric_limits<int32_t>::min())
    {
        return {};
    }
    if(numTokens < 0)
    {
        numTokens = -numTokens;
    }

    std::vector<llama_token> tokens(static_cast<size_t>(numTokens));
    auto const tokenizedCount = llama_tokenize(vocab, utf8Text, static_cast<int32_t>(utf8Size), tokens.data(), numTokens, true, true);
    MiscWeakAssert(tokenizedCount > 0);
    if(tokenizedCount <= 0)
    {
        return {};
    }

    auto* context = mEmbeddingContext.get();
    if(auto* memory = llama_get_memory(context))
    {
        llama_memory_clear(memory, true);
    }
    auto batch = llama_batch_get_one(tokens.data(), tokenizedCount);
    auto const status = llama_model_has_encoder(model) ? llama_encode(context, batch) : llama_decode(context, batch);
    MiscWeakAssert(status == 0);
    if(status != 0)
    {
        return {};
    }
    llama_synchronize(context);

    auto const* embeddingData = [&]()
    {
        auto const* data = llama_get_embeddings_seq(context, 0);
        return data != nullptr ? data : llama_get_embeddings_ith(context, tokenizedCount - 1);
    }();
    MiscWeakAssert(embeddingData != nullptr);
    if(embeddingData == nullptr)
    {
        return {};
    }

    auto const nEmbd = llama_model_n_embd_out(mEmbeddingModel.get());
    std::vector<float> embedding{embeddingData, embeddingData + nEmbd};
    // Normalize the embedding to unit length
    auto const norm = std::sqrt(std::accumulate(embedding.cbegin(), embedding.cend(), 0.0f, [](float sum, float value)
                                                {
                                                    return sum + value * value;
                                                }));
    MiscWeakAssert(norm > std::numeric_limits<float>::epsilon());
    if(norm <= std::numeric_limits<float>::epsilon())
    {
        return {};
    }
    for(auto& value : embedding)
    {
        value /= norm;
    }
    return embedding;
}

juce::Result Application::Neuralyzer::Rag::Engine::setResources(std::set<Resource> resources)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mResourceEntries = std::move(resources);

    MiscWeakAssert(mEmbeddingModel != nullptr && mEmbeddingContext != nullptr);
    if(mEmbeddingModel == nullptr || mEmbeddingContext == nullptr)
    {
        return juce::Result::fail(juce::translate("RAG model is not initialized."));
    }

    mIndex.reset();
    mIndexedEntries.clear();
    if(mResourceEntries.empty())
    {
        return juce::Result::ok();
    }

    mIndexedEntries.reserve(mResourceEntries.size());

    auto const numEmbd = llama_model_n_embd_out(mEmbeddingModel.get());
    std::vector<float> matrix;
    matrix.reserve(mResourceEntries.size() * static_cast<size_t>(numEmbd));

    for(auto const& chunk : mResourceEntries)
    {
        auto const textForEmbedding = juce::String("[") + chunk.document + " > " + chunk.section + "]\n" + chunk.content;
        auto const embedding = computeEmbedding(textForEmbedding);
        if(!embedding.empty())
        {
            matrix.insert(matrix.end(), embedding.begin(), embedding.end());
            mIndexedEntries.push_back(chunk);
        }
    }
    if(mIndexedEntries.empty())
    {
        return juce::Result::fail(juce::translate("No valid resource entries to index."));
    }

    mSpace = std::make_unique<hnswlib::InnerProductSpace>(numEmbd);
    mIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(mSpace.get(), static_cast<size_t>(mIndexedEntries.size()), 16, 100);
    auto const dim = static_cast<size_t>(numEmbd);
    for(size_t i = 0; i < mIndexedEntries.size(); ++i)
    {
        mIndex->addPoint(matrix.data() + (i * dim), i);
    }
    mIndex->setEf(std::clamp(mIndexedEntries.size(), 32_z, 128_z));
    return juce::Result::ok();
}

std::vector<Application::Neuralyzer::Rag::Context> Application::Neuralyzer::Rag::Engine::computeContext(juce::String const& userPrompt, float maxDistance)
{
    auto const trimmedPrompt = userPrompt.trim();
    if(trimmedPrompt.isEmpty())
    {
        return {};
    }

    std::lock_guard<std::mutex> lock(mMutex);
    if(mIndex == nullptr || mIndexedEntries.empty())
    {
        return {};
    }

    auto const queryEmbedding = computeEmbedding(trimmedPrompt);
    if(queryEmbedding.empty())
    {
        return {};
    }

    auto const topK = std::min(6_z, mIndexedEntries.size());
    auto matches = mIndex->searchKnn(queryEmbedding.data(), topK);
    std::vector<std::tuple<float, size_t, size_t>> ranked;
    ranked.reserve(topK);
    while(!matches.empty())
    {
        auto const& [distance, label] = matches.top();
        auto const index = ranked.size();
        ranked.push_back({distance, label, index});
        matches.pop();
    }
    std::sort(ranked.begin(), ranked.end(), [](auto const& lhs, auto const& rhs)
              {
                  return std::get<0>(lhs) < std::get<0>(rhs);
              });

    std::vector<Context> contexts;
    std::set<size_t> addedIndices;
    static constexpr int maxContexts = 3;

    auto const addContext = [&](size_t index)
    {
        if(index >= mIndexedEntries.size() || addedIndices.count(index) > 0)
        {
            return;
        }
        auto const& resource = mIndexedEntries.at(index);
        contexts.emplace_back(Context{resource.content, resource.section, resource.document});
        addedIndices.insert(index);
    };

    for(auto const& [distance, label, index] : ranked)
    {
        if(distance > maxDistance || contexts.size() >= maxContexts)
        {
            break;
        }
        addContext(label);
        if(index > 0)
        {
            auto const it = std::find_if(ranked.cbegin(), ranked.cend(), [&](auto const& rank)
                                         {
                                             return std::get<2>(rank) == index - 1;
                                         });
            if(it != ranked.cend() && std::get<0>(*it) <= maxDistance)
            {
                addContext(std::get<1>(*it));
            }
        }
        if(index + 1 < ranked.size())
        {
            auto const it = std::find_if(ranked.cbegin(), ranked.cend(), [&](auto const& rank)
                                         {
                                             return std::get<2>(rank) == index + 1;
                                         });
            if(it != ranked.cend() && std::get<0>(*it) <= maxDistance)
            {
                addContext(std::get<1>(*it));
            }
        }
    }
    return contexts;
}

ANALYSE_FILE_END
