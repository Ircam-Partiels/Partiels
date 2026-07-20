#include "AnlApplicationNeuralyzerRag.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

void Application::Neuralyzer::Rag::to_json(nlohmann::json& json, Application::Neuralyzer::Rag::Resource const& resource)
{
    // clang-format off
    json = nlohmann::json
    {
          {"id", resource.id}
        , {"content", resource.content}
        , {"section", resource.section}
        , {"document", resource.document}
        , {"score", resource.score}
    };
    // clang-format on
}

void Application::Neuralyzer::Rag::from_json(nlohmann::json const& json, Application::Neuralyzer::Rag::Resource& resource)
{
    resource.id = json.value("id", resource.id).trim();
    resource.content = json.value("content", resource.content).trim();
    resource.section = json.value("section", resource.section).trim();
    resource.document = json.value("document", resource.document).trim();
    resource.score = json.value("score", resource.score);
}

juce::File Application::Neuralyzer::Rag::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Rag");
}

juce::URL Application::Neuralyzer::Rag::getEmbeddingModelUrl()
{
    return juce::URL("https://huggingface.co/gpustack/bge-m3-GGUF/resolve/main/bge-m3-Q4_K_M.gguf");
}

juce::URL Application::Neuralyzer::Rag::getRerankerModelUrl()
{
    return juce::URL("https://huggingface.co/gpustack/bge-reranker-v2-m3-GGUF/resolve/main/bge-reranker-v2-m3-Q4_K_M.gguf");
}

juce::File Application::Neuralyzer::Rag::getEmbeddingModelFile()
{
    return getDefaultModelDirectory().getChildFile(getEmbeddingModelUrl().getFileName());
}

juce::File Application::Neuralyzer::Rag::getRerankerModelFile()
{
    return getDefaultModelDirectory().getChildFile(getRerankerModelUrl().getFileName());
}

void Application::Neuralyzer::Rag::downloadModelsIfNecessary()
{
    static auto const embeddingModelFile = getEmbeddingModelFile();
    static auto const rerankerModelFile = getRerankerModelFile();
    auto const callback = [=](Downloader::Process const& process)
    {
        if(process.getResult().failed())
        {
            return;
        }
        auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
        // Ensure only when the 2 models are available the user is notified
        if(!embeddingModelFile.existsAsFile() || downloader.isDownloading(embeddingModelFile) || !rerankerModelFile.existsAsFile() || downloader.isDownloading(rerankerModelFile))
        {
            return;
        }

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("RAG Models Downloaded"))
                                 .withMessage(juce::translate("The RAG models have been successfully downloaded. You must restart the application to enable the RAG system."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    };

    auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
    if(!embeddingModelFile.existsAsFile())
    {
        downloader.start(embeddingModelFile, getEmbeddingModelUrl(), callback);
    }
    if(!rerankerModelFile.existsAsFile())
    {
        downloader.start(rerankerModelFile, getRerankerModelUrl(), callback);
    }
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

std::vector<Application::Neuralyzer::Rag::Resource> Application::Neuralyzer::Rag::createResources(Ast::Document const& document, int maxResourceLength)
{
    auto const slugify = [](juce::String const& value)
    {
        juce::String out;
        auto lastWasDash = false;
        for(auto const character : value.toLowerCase().trim())
        {
            if(juce::CharacterFunctions::isLetterOrDigit(character))
            {
                out << character;
                lastWasDash = false;
            }
            else if(!std::exchange(lastWasDash, true))
            {
                out << "-";
            }
        }
        return out.trimCharactersAtStart("-").trimCharactersAtEnd("-");
    };

    std::vector<Resource> resources;
    resources.reserve(document.sections.size());
    for(auto const& section : document.sections)
    {
        Resource resource;
        resource.document = document.document;
        juce::StringArray ancestors;
        for(auto const& ancestor : section.hierarchy)
        {
            ancestors.add(ancestor);
        }
        resource.section = ancestors.joinIntoString(" > ");
        int resourceIndex = 1;
        auto const addResource = [&]()
        {
            if(!resource.content.isEmpty())
            {
                resource.id = slugify(resource.section + "-" + juce::String(resourceIndex));
                resources.push_back(resource);
                resource.content.clear();
                ++resourceIndex;
            }
        };

        auto lines = juce::StringArray::fromLines(section.content.replace("\r", ""));
        lines.trim();
        lines.removeEmptyStrings();
        for(auto const& line : lines)
        {
            if(resource.content.length() + line.length() > maxResourceLength)
            {
                addResource();
            }
            if(line.length() > maxResourceLength)
            {
                addResource();
                auto tokens = juce::StringArray::fromTokens(" ", true);
                for(auto const& token : tokens)
                {
                    if(resource.content.length() + token.length() > maxResourceLength)
                    {
                        addResource();
                    }
                    if(token.length() > maxResourceLength)
                    {
                        addResource();
                        resource.content = token;
                        addResource();
                    }
                    else
                    {
                        resource.content += token + " ";
                    }
                }
            }
            else
            {
                resource.content += line + "\n";
            }
        }
        addResource();
    }
    return resources;
}

Application::Neuralyzer::Rag::Engine::Engine(Mcp::Dispatcher& mcpDispatcher)
: mMcpDispatcher(mcpDispatcher)
{
}

juce::Result Application::Neuralyzer::Rag::Engine::initializeModels(juce::File embeddingModelFile, juce::File rerankerModelFile)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mEmbeddingContext.reset();
    mEmbeddingModel.reset();
    if(!embeddingModelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("RAG embedding model file does not exist: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("RAG embedding model file does not exist: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
    }
    if(!rerankerModelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("RAG reranker model file does not exist: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("RAG reranker model file does not exist: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
    }

    auto modelParams = llama_model_default_params();
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;
    mEmbeddingModel.reset(llama_model_load_from_file(embeddingModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mEmbeddingModel == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
    }

    if(!llama_model_has_encoder(mEmbeddingModel.get()) && !llama_model_has_decoder(mEmbeddingModel.get()))
    {
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG embedding model does not support encoder nor decoder.");
        return juce::Result::fail(juce::translate("RAG embedding model does not support encoder nor decoder."));
    }

    auto contextParams = llama_context_default_params();
    contextParams.n_ctx = 2048;
    contextParams.n_batch = 512;
    contextParams.n_seq_max = 1;
    contextParams.embeddings = true;
    contextParams.pooling_type = LLAMA_POOLING_TYPE_UNSPECIFIED;
    contextParams.no_perf = true;

    mEmbeddingContext.reset(llama_init_from_model(mEmbeddingModel.get(), contextParams));
    if(mEmbeddingContext == nullptr)
    {
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "Failed to initialize RAG embedding model context.");
        return juce::Result::fail(juce::translate("Failed to initialize RAG embedding model context."));
    }

    if(llama_model_n_embd_out(mEmbeddingModel.get()) <= 0)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG embedding model embedding size is invalid.");
        return juce::Result::fail(juce::translate("RAG embedding model embedding size is invalid."));
    }

    contextParams.n_batch = contextParams.n_ctx;
    contextParams.embeddings = true;
    contextParams.pooling_type = LLAMA_POOLING_TYPE_RANK;
    mRerankerModel.reset(llama_model_load_from_file(rerankerModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mRerankerModel == nullptr)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
    }

    if(!llama_model_has_encoder(mRerankerModel.get()) && !llama_model_has_decoder(mRerankerModel.get()))
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG reranker model does not support encoder nor decoder.");
        return juce::Result::fail(juce::translate("RAG reranker model does not support encoder nor decoder."));
    }
    contextParams.n_ctx = 4096;
    contextParams.embeddings = false;
    mRerankerContext.reset(llama_init_from_model(mRerankerModel.get(), contextParams));
    if(mRerankerContext == nullptr)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        mRerankerModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "Failed to initialize RAG reranker model context.");
        return juce::Result::fail(juce::translate("Failed to initialize RAG reranker model context."));
    }

    if(llama_pooling_type(mRerankerContext.get()) != LLAMA_POOLING_TYPE_RANK)
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", "The context does not use RANK pooling. Verify that the GGUF actually contains cls.output.weight.");
        return juce::Result::fail(juce::translate("The context does not use RANK pooling."));
    }

    mIndex.reset();
    mIndexedEntries.clear();
    std::vector<Application::Neuralyzer::Rag::Resource> allResources;
    for(auto const& [name, text] : mMcpDispatcher.getResources())
    {
        auto const ast = Neuralyzer::Ast::parseMarkdown(text, name, false);
        auto const resources = Neuralyzer::Rag::createResources(ast, 1200);
        allResources.insert(allResources.end(), resources.cbegin(), resources.cend());
    }
    return setResources(allResources);
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
    auto const batchSize = static_cast<size_t>(llama_n_batch(context));
    if(batchSize == 0)
    {
        return {};
    }

    size_t position = 0;
    int32_t lastBatchTokenCount = 0;
    while(position < static_cast<size_t>(tokenizedCount))
    {
        auto const chunkSize = std::min(batchSize, static_cast<size_t>(tokenizedCount) - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(chunkSize));
        if(llama_encode(context, batch) != 0)
        {
            auto const status = llama_decode(context, batch);
            MiscWeakAssert(status == 0);
            if(status != 0)
            {
                return {};
            }
        }
        lastBatchTokenCount = static_cast<int32_t>(chunkSize);
        position += chunkSize;
    }
    llama_synchronize(context);

    auto const* embeddingData = [&]()
    {
        auto const* data = llama_get_embeddings_seq(context, 0);
        return data != nullptr ? data : llama_get_embeddings_ith(context, lastBatchTokenCount - 1);
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

float Application::Neuralyzer::Rag::Engine::computeRerankerScore(juce::String const& query, juce::String const& resourceContent)
{
    MiscWeakAssert(mRerankerModel != nullptr && mRerankerContext != nullptr);
    if(mRerankerContext == nullptr)
    {
        return 0.0f;
    }
    auto* context = mRerankerContext.get();
    auto* model = mRerankerModel.get();
    auto const* vocab = llama_model_get_vocab(model);

    auto const sepToken = llama_vocab_get_add_sep(vocab) ? std::string(llama_vocab_get_text(vocab, llama_vocab_sep(vocab))) : std::string{};
    auto const eosToken = llama_vocab_get_add_eos(vocab) ? std::string(llama_vocab_get_text(vocab, llama_vocab_eos(vocab))) : std::string{};
    auto const finalPrompt = query.toStdString() + eosToken + sepToken + resourceContent.toStdString();
    auto tokens = common_tokenize(context, finalPrompt, true, true);

    auto const numBatch = static_cast<size_t>(llama_n_batch(context));
    MiscWeakAssert(tokens.size() <= numBatch);
    if(tokens.size() > numBatch)
    {
        tokens.resize(numBatch);
    }

    auto batch = llama_batch_init(static_cast<int32_t>(tokens.size()), 0, 1);
    for(auto j = 0_z; j < tokens.size(); ++j)
    {
        common_batch_add(batch, tokens[j], static_cast<llama_pos>(j), {0}, true);
    }

    // Clear the memory cache before each new pair (no shared context between pairs)
    llama_memory_clear(llama_get_memory(context), true);

    llama_set_embeddings(context, true);
    if(llama_decode(context, batch) < 0)
    {
        MiscWeakAssert(false);
        llama_batch_free(batch);
        return 0.0f;
    }

    // With RANK pooling, a single score is returned for sequence 0
    auto const* embd = llama_get_embeddings_seq(context, 0);
    if(embd == nullptr)
    {
        return 0.0f;
    }
    auto const relevance = 1.0f / (1.0f + std::exp(-embd[0]));
    llama_batch_free(batch);
    return relevance;
}

juce::Result Application::Neuralyzer::Rag::Engine::setResources(std::vector<Resource> const& resources)
{
    MiscWeakAssert(mEmbeddingModel != nullptr && mEmbeddingContext != nullptr);
    if(mEmbeddingModel == nullptr || mEmbeddingContext == nullptr)
    {
        return juce::Result::fail(juce::translate("RAG model is not initialized."));
    }

    mIndex.reset();
    mIndexedEntries.clear();
    if(resources.empty())
    {
        return juce::Result::ok();
    }

    mIndexedEntries.reserve(resources.size());

    auto const numEmbd = llama_model_n_embd_out(mEmbeddingModel.get());
    std::vector<float> matrix;
    matrix.reserve(resources.size() * static_cast<size_t>(numEmbd));

    for(auto const& chunk : resources)
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

std::vector<Application::Neuralyzer::Rag::Resource> Application::Neuralyzer::Rag::Engine::computeResources(juce::String const& userPrompt, size_t maxResources, float minScore, float maxDistance)
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

    auto const topK = std::min(30_z, mIndexedEntries.size());
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

    std::vector<Resource> resources;
    std::set<size_t> addedIndices;

    auto const addResource = [&](size_t index)
    {
        if(index >= mIndexedEntries.size() || addedIndices.count(index) > 0)
        {
            return;
        }
        auto const& resource = mIndexedEntries.at(index);
        resources.push_back(resource);
        addedIndices.insert(index);
    };

    for(auto const& [distance, label, index] : ranked)
    {
        if(distance > maxDistance)
        {
            break;
        }
        addResource(label);
        if(index > 0)
        {
            auto const it = std::find_if(ranked.cbegin(), ranked.cend(), [i = index - 1](auto const& rank)
                                         {
                                             return std::get<2>(rank) == i;
                                         });
            if(it != ranked.cend() && std::get<0>(*it) <= maxDistance)
            {
                addResource(std::get<1>(*it));
            }
        }
        if(index + 1 < ranked.size())
        {
            auto const it = std::find_if(ranked.cbegin(), ranked.cend(), [i = index + 1](auto const& rank)
                                         {
                                             return std::get<2>(rank) == i;
                                         });
            if(it != ranked.cend() && std::get<0>(*it) <= maxDistance)
            {
                addResource(std::get<1>(*it));
            }
        }
    }

    // Re-rank resources using the reranker model if available
    if(mRerankerModel != nullptr && mRerankerContext != nullptr)
    {
        for(auto& resource : resources)
        {
            resource.score = computeRerankerScore(trimmedPrompt, resource.content);
        }

        // Sort by reranker score (descending)
        std::sort(resources.begin(), resources.end(), [](auto const& lhs, auto const& rhs)
                  {
                      return lhs.score > rhs.score;
                  });

        // Keep only confident matches
        resources.resize(maxResources);
        std::erase_if(resources, [&](auto const& resource)
                      {
                          return resource.score < minScore;
                      });
    }
    return resources;
}

std::vector<Application::Neuralyzer::Rag::Resource> Application::Neuralyzer::Rag::Engine::getResources(std::vector<juce::String> const& ids) const
{
    std::vector<Resource> resources;
    resources.reserve(ids.size());
    std::copy_if(mIndexedEntries.cbegin(), mIndexedEntries.cend(), std::back_inserter(resources), [&](auto const& resource)
                 {
                     return std::find(ids.cbegin(), ids.cend(), resource.id) != ids.cend();
                 });
    return resources;
}

ANALYSE_FILE_END
