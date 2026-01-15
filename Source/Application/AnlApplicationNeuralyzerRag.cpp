#include "AnlApplicationNeuralyzerRag.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerModel.h"

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

juce::File Application::Neuralyzer::Rag::getDefaultModelDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Rag");
}

juce::URL Application::Neuralyzer::Rag::getEmbeddingModelUrl()
{
    return juce::URL(PARTIELS_NEURALYZER_RAG_EMBEDDING_MODEL_URL);
}

juce::URL Application::Neuralyzer::Rag::getRerankerModelUrl()
{
    return juce::URL(PARTIELS_NEURALYZER_RAG_RERANKER_MODEL_URL);
}

juce::File Application::Neuralyzer::Rag::getEmbeddingModelFile()
{
    return getDefaultModelDirectory().getChildFile(getEmbeddingModelUrl().getFileName());
}

juce::File Application::Neuralyzer::Rag::getRerankerModelFile()
{
    return getDefaultModelDirectory().getChildFile(getRerankerModelUrl().getFileName());
}

void Application::Neuralyzer::Rag::downloadRagModelsIfNecessary()
{
    auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
    auto const callback = [](Downloader::Process const& process)
    {
        if(process.getResult().failed())
        {
            return;
        }
        // Ensure only when the 2 models are available the user is notified
        auto const& processes = Instance::get().getNeuralyzerDownloaderManager().getProcesses();
        if(std::any_of(processes.cbegin(), processes.cend(), [](auto const& proc)
                       {
                           return proc.get().isRunning() && (proc.get().getOutputFile() == getEmbeddingModelFile() || proc.get().getOutputFile() == getRerankerModelFile());
                       }))
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

    if(!getEmbeddingModelFile().existsAsFile())
    {
        downloader.start(getDefaultModelDirectory(), getEmbeddingModelUrl(), callback);
    }
    if(!getRerankerModelFile().existsAsFile())
    {
        downloader.start(getDefaultModelDirectory(), getRerankerModelUrl(), callback);
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
                resource.id = Formatter::slugify(resource.section + "-" + juce::String(resourceIndex));
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
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("RAG embedding model file does not exist: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("RAG embedding model file does not exist: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
    }
    if(!rerankerModelFile.existsAsFile())
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("RAG reranker model file does not exist: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("RAG reranker model file does not exist: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
    }

    auto modelParams = llama_model_default_params();
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;
    mEmbeddingModel.reset(llama_model_load_from_file(embeddingModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mEmbeddingModel == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
    }

    if(!llama_model_has_encoder(mEmbeddingModel.get()) && !llama_model_has_decoder(mEmbeddingModel.get()))
    {
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("RAG embedding model does not support encoder nor decoder."));
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
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("Failed to initialize RAG embedding model context."));
        return juce::Result::fail(juce::translate("Failed to initialize RAG embedding model context."));
    }

    if(llama_model_n_embd_out(mEmbeddingModel.get()) <= 0)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("RAG embedding model embedding size is invalid."));
        return juce::Result::fail(juce::translate("RAG embedding model embedding size is invalid."));
    }

    mRerankerModel.reset(llama_model_load_from_file(rerankerModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mRerankerModel == nullptr)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
    }

    if(!llama_model_has_encoder(mRerankerModel.get()) && !llama_model_has_decoder(mRerankerModel.get()))
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        mEmbeddingModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("RAG reranker model does not support encoder nor decoder."));
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
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("Failed to initialize RAG reranker model context."));
        return juce::Result::fail(juce::translate("Failed to initialize RAG reranker model context."));
    }

    auto const* vocab = llama_model_get_vocab(mRerankerModel.get());
    auto const resolveLabelToken = [vocab](std::initializer_list<char const*> candidates)
    {
        for(auto const* candidate : candidates)
        {
            auto const candidateText = juce::String(candidate);
            auto const* candidateUtf8 = candidateText.toRawUTF8();
            auto const candidateUtf8Size = candidateText.getNumBytesAsUTF8();
            auto tokenCount = llama_tokenize(vocab, candidateUtf8, static_cast<int32_t>(candidateUtf8Size), nullptr, 0, false, true);
            if(tokenCount == 0 || tokenCount == std::numeric_limits<int32_t>::min())
            {
                continue;
            }
            if(tokenCount < 0)
            {
                tokenCount = -tokenCount;
            }

            std::vector<llama_token> tokenBuffer(static_cast<size_t>(tokenCount));
            auto const actualCount = llama_tokenize(vocab, candidateUtf8, static_cast<int32_t>(candidateUtf8Size), tokenBuffer.data(), tokenCount, false, true);
            if(actualCount > 0)
            {
                return tokenBuffer.at(static_cast<size_t>(actualCount - 1));
            }
        }
        return static_cast<llama_token>(-1);
    };
    auto const vocabSize = llama_vocab_n_tokens(vocab);
    mRerankerYesToken = resolveLabelToken({" Yes", "Yes", "yes", " True", "True", "true"});
    mRerankerNoToken = resolveLabelToken({" No", "No", "no", " False", "False", "false"});
    if(mRerankerYesToken < 0 || mRerankerNoToken < 0 || mRerankerYesToken >= vocabSize || mRerankerNoToken >= vocabSize)
    {
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        mRerankerContext.reset();
        mRerankerModel.reset();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::translate("Failed to resolve RAG reranker model label tokens."));
        return juce::Result::fail(juce::translate("Failed to resolve RAG reranker model label tokens."));
    }

    mIndex.reset();
    mIndexedEntries.clear();
    std::vector<Application::Neuralyzer::Rag::Resource> allResources;
    for(auto const& [name, text] : mMcpDispatcher.getResources())
    {
        auto const ast = Neuralyzer::Ast::parseMarkdown(text, name, false);
        auto const resources = Neuralyzer::Rag::createResources(ast, 700);
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
        auto const status = llama_model_has_encoder(model) ? llama_encode(context, batch) : llama_decode(context, batch);
        MiscWeakAssert(status == 0);
        if(status != 0)
        {
            return {};
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

    // Query context followed by resource
    auto const pairText = "Query: " + query + "\nDocument: " + resourceContent + "\nRelevant:";
    auto* model = mRerankerModel.get();

    auto const* utf8Text = pairText.toRawUTF8();
    auto const utf8Size = pairText.getNumBytesAsUTF8();
    auto const* vocab = llama_model_get_vocab(model);
    auto numTokens = llama_tokenize(vocab, utf8Text, static_cast<int32_t>(utf8Size), nullptr, 0, true, true);
    MiscWeakAssert(numTokens != 0 && numTokens != std::numeric_limits<int32_t>::min());
    if(numTokens == 0 || numTokens == std::numeric_limits<int32_t>::min())
    {
        return 0.0f;
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
        return 0.0f;
    }

    auto* context = mRerankerContext.get();
    if(auto* memory = llama_get_memory(context))
    {
        llama_memory_clear(memory, true);
    }
    auto const batchSize = static_cast<size_t>(llama_n_batch(context));
    if(batchSize == 0)
    {
        return 0.0f;
    }

    size_t position = 0;
    int32_t lastBatchTokenCount = 0;
    while(position < static_cast<size_t>(tokenizedCount))
    {
        auto const chunkSize = std::min(batchSize, static_cast<size_t>(tokenizedCount) - position);
        auto const batch = llama_batch_get_one(tokens.data() + position, static_cast<int32_t>(chunkSize));
        auto const status = llama_decode(context, batch);
        MiscWeakAssert(status == 0);
        if(status != 0)
        {
            return 0.0f;
        }
        lastBatchTokenCount = static_cast<int32_t>(chunkSize);
        position += chunkSize;
    }
    llama_synchronize(context);

    auto const* logits = llama_get_logits_ith(context, lastBatchTokenCount - 1);
    if(logits == nullptr)
    {
        return 0.0f;
    }

    // Numerically stable softmax over the Yes/No pair.
    return 1.0f / (1.0f + std::exp(logits[mRerankerYesToken] - logits[mRerankerNoToken]));
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
        std::vector<std::pair<float, Resource>> rankedByReranker;
        rankedByReranker.reserve(resources.size());

        for(auto const& resource : resources)
        {
            auto const rerankerScore = computeRerankerScore(trimmedPrompt, resource.content);
            rankedByReranker.push_back({rerankerScore, resource});
        }

        // Sort by reranker score (descending)
        std::sort(rankedByReranker.begin(), rankedByReranker.end(), [](auto const& lhs, auto const& rhs)
                  {
                      return lhs.first > rhs.first;
                  });

        // Keep only confident matches
        resources.clear();
        for(auto i = 0_z; i < rankedByReranker.size() && resources.size() < maxResources; ++i)
        {
            auto const& [score, resource] = rankedByReranker.at(i);
            if(score >= minScore)
            {
                resources.push_back(resource);
            }
        }
    }
    return resources;
}

ANALYSE_FILE_END
