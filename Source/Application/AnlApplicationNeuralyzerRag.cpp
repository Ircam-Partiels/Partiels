#include "AnlApplicationNeuralyzerRag.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationNeuralyzerModel.h"
#include <AnlNeuralyzerData.h>
#include <AnlNeuralyzerTutorialData.h>
#include <common.h>

ANALYSE_FILE_BEGIN

namespace
{
    // A segment of text and whether its special tokens (such as </s>) must be parsed
    using TextSegment = std::pair<std::string, bool>;

    // Tokenizes text segments like a single text with add_special = true, but only parses the special tokens of the
    // segments that allow it, so that a query or a document containing the text of a special token cannot alter the
    // structure of the sequence
    std::vector<llama_token> tokenizeSegments(llama_vocab const* vocab, std::vector<TextSegment> const& segments)
    {
        // The special tokens that the tokenizer adds around a text depend on the vocabulary (BOS/EOS, CLS/SEP...)
        auto const specialTokens = common_tokenize(vocab, "", true, false);
        auto const hasPrefix = !specialTokens.empty() && specialTokens.front() == llama_vocab_bos(vocab);

        std::vector<llama_token> tokens;
        if(hasPrefix)
        {
            tokens.push_back(specialTokens.front());
        }
        for(auto const& [text, parseSpecial] : segments)
        {
            auto const segmentTokens = common_tokenize(vocab, text, false, parseSpecial);
            tokens.insert(tokens.end(), segmentTokens.cbegin(), segmentTokens.cend());
        }
        tokens.insert(tokens.end(), std::next(specialTokens.cbegin(), hasPrefix ? 1 : 0), specialTokens.cend());
        return tokens;
    }

    // The text of a resource with its document and its section, used for the embedding and the reranking because the
    // content of a chunk often depends on its heading (for instance, a "Window Size" section that only describes the effect)
    juce::String getTextWithContext(Application::Neuralyzer::Rag::Resource const& resource)
    {
        return juce::String("[") + resource.document + " > " + resource.section + "]\n" + resource.content;
    }

    // The MCP response of the tools search_docs and load_docs
    nlohmann::json createTextResponse(std::string const& text, bool isError)
    {
        nlohmann::json response;
        response["isError"] = isError;
        response["content"] = nlohmann::json::array();
        nlohmann::json content;
        content["type"] = "text";
        content["text"] = text;
        response["content"].push_back(content);
        return response;
    }
} // namespace

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

juce::File Application::Neuralyzer::Rag::getContextDirectory()
{
    auto const root = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return resolveDirectory(root).getChildFile("Context");
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
        // Ensure only when the 2 models are available the models are loaded and the user is notified
        if(!embeddingModelFile.existsAsFile() || downloader.isDownloading(embeddingModelFile) || !rerankerModelFile.existsAsFile() || downloader.isDownloading(rerankerModelFile))
        {
            return;
        }
        Instance::get().getNeuralyzerAgent().reloadRagModels();

        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::AlertIconType::InfoIcon)
                                 .withTitle(juce::translate("RAG Models Downloaded"))
                                 .withMessage(juce::translate("The RAG models have been successfully downloaded. The RAG system is being enabled."))
                                 .withButton(juce::translate("Ok"), 0);
        juce::AlertWindow::showAsync(options, nullptr);
    };

    auto& downloader = Instance::get().getNeuralyzerDownloaderManager();
    downloader.startIfNecessary(embeddingModelFile, getEmbeddingModelUrl(), callback);
    downloader.startIfNecessary(rerankerModelFile, getRerankerModelUrl(), callback);
}

void Application::Neuralyzer::Rag::askToDownloadModels(std::function<void()> callback)
{
    auto& accessor = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::neuralyzer>();
    // The models downloaded by a previous version (before the user was asked) are kept
    if(accessor.getAttr<AttrType::ragModelsDownload>() == DownloadChoice::undecided && (getEmbeddingModelFile().existsAsFile() || getRerankerModelFile().existsAsFile()))
    {
        accessor.setAttr<AttrType::ragModelsDownload>(DownloadChoice::accepted, NotificationType::synchronous);
    }

    switch(accessor.getAttr<AttrType::ragModelsDownload>())
    {
        case DownloadChoice::declined:
            break;
        case DownloadChoice::accepted:
        {
            downloadModelsIfNecessary();
            break;
        }
        case DownloadChoice::undecided:
        {
            // The result 0 (remind me later) is also used when the window is closed with the escape key
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::AlertIconType::QuestionIcon)
                                     .withTitle(juce::translate("Download the RAG Models?"))
                                     .withMessage(juce::translate("The RAG models (an embedding model and a reranker model, about 900 MB) allow the agent of the chat and the MCP server to search the documentation of Partiels, which improves the answers about the use of the application. Without these models, the documentation cannot be searched. The models can also be downloaded later from the agent settings."))
                                     .withButton(juce::translate("Download"), 2)
                                     .withButton(juce::translate("Don't download"), 1)
                                     .withButton(juce::translate("Remind me later"), 0);
            juce::AlertWindow::showAsync(options, [=](int result)
                                         {
                                             auto& acsr = Instance::get().getApplicationAccessor().getAcsr<Application::AcsrType::neuralyzer>();
                                             if(result == 2)
                                             {
                                                 acsr.setAttr<AttrType::ragModelsDownload>(DownloadChoice::accepted, NotificationType::synchronous);
                                                 downloadModelsIfNecessary();
                                             }
                                             else if(result == 1)
                                             {
                                                 acsr.setAttr<AttrType::ragModelsDownload>(DownloadChoice::declined, NotificationType::synchronous);
                                             }
                                             if(callback != nullptr)
                                             {
                                                 callback();
                                             }
                                         });
            return;
        }
    }
    if(callback != nullptr)
    {
        callback();
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

    // The identifiers are copied by the model from search_docs to load_docs, so they must be short and readable: the
    // document and the last heading of the section (the first heading is ignored when it is the title of the document),
    // all the headings when the last heading is not unique in the document, and a numeric suffix as a last resort
    auto const documentSlug = slugify(document.document);
    auto const joinSlugs = [](juce::StringArray slugs)
    {
        slugs.removeEmptyStrings();
        return slugs.joinIntoString("-");
    };
    std::vector<juce::String> shortBases;
    std::vector<juce::String> fullBases;
    for(auto const& section : document.sections)
    {
        juce::StringArray headings;
        for(auto const& heading : section.hierarchy)
        {
            headings.add(slugify(heading));
        }
        if(!headings.isEmpty() && headings[0] == documentSlug)
        {
            headings.remove(0);
        }
        headings.removeEmptyStrings();
        shortBases.push_back(joinSlugs({documentSlug, headings.isEmpty() ? juce::String() : headings[headings.size() - 1]}));
        fullBases.push_back(joinSlugs({documentSlug, headings.joinIntoString("-")}));
    }
    // An identifier is a base and an index without dash, so unique bases give unique identifiers
    std::vector<juce::String> bases;
    std::set<juce::String> usedBases;
    for(size_t i = 0_z; i < document.sections.size(); ++i)
    {
        auto const& base = std::count(shortBases.cbegin(), shortBases.cend(), shortBases.at(i)) > 1 ? fullBases.at(i) : shortBases.at(i);
        auto uniqueBase = base;
        for(auto suffix = 2; !usedBases.insert(uniqueBase).second; ++suffix)
        {
            uniqueBase = base + "-" + juce::String(suffix);
        }
        bases.push_back(uniqueBase);
    }

    std::vector<Resource> resources;
    resources.reserve(document.sections.size());
    for(size_t sectionIndex = 0_z; sectionIndex < document.sections.size(); ++sectionIndex)
    {
        auto const& section = document.sections.at(sectionIndex);
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
            // The trailing separator (new line or space) is removed so the content does not exceed the maximum length,
            // and the content is trimmed before checking that it is not empty (a section can contain only empty lines)
            resource.content = resource.content.trimCharactersAtEnd("\n ").trimCharactersAtStart("\n");
            if(!resource.content.isEmpty())
            {
                resource.id = bases.at(sectionIndex) + "-" + juce::String(resourceIndex);
                resources.push_back(resource);
                resource.content.clear();
                ++resourceIndex;
            }
        };

        auto const lines = juce::StringArray::fromLines(section.content.replace("\r", ""));
        for(auto const& line : lines)
        {
            if(resource.content.length() + line.length() > maxResourceLength)
            {
                addResource();
            }
            if(line.length() > maxResourceLength)
            {
                addResource();
                auto const tokens = juce::StringArray::fromTokens(line, " ", "");
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

juce::Result Application::Neuralyzer::Rag::Engine::initializeModels(juce::File embeddingModelFile, juce::File rerankerModelFile)
{
    std::lock_guard<std::mutex> lock(mMutex);

    // The index is computed with the embedding model, and the contexts must be released before the models they reference
    auto const releaseModels = [this]()
    {
        mIndex.reset();
        mIndexedEntries.clear();
        mRerankerContext.reset();
        mRerankerModel.reset();
        mEmbeddingContext.reset();
        mEmbeddingModel.reset();
        mEmbeddingUsesEncoder = false;
        mRerankerUsesEncoder = false;
    };

    releaseModels();
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
    modelParams.load_mode = llama_load_mode::LLAMA_LOAD_MODE_MMAP;
    modelParams.progress_callback_user_data = static_cast<void*>(this);
    modelParams.progress_callback = [](float, void* data) -> bool
    {
        return !reinterpret_cast<Engine*>(data)->mShouldQuit.load();
    };
    mEmbeddingModel.reset(llama_model_load_from_file(embeddingModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mEmbeddingModel == nullptr)
    {
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG embedding model: FLNAME").replace("FLNAME", embeddingModelFile.getFullPathName()));
    }

    if(llama_model_has_encoder(mEmbeddingModel.get()) && llama_model_has_decoder(mEmbeddingModel.get()))
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG embedding model with an encoder and a decoder is not supported.");
        return juce::Result::fail(juce::translate("RAG embedding model with an encoder and a decoder is not supported."));
    }

    auto contextParams = llama_context_default_params();
    // Non-causal models (encoders, rerankers) must process all the tokens in a single micro-batch
    contextParams.n_ctx = 2048;
    contextParams.n_batch = contextParams.n_ctx;
    contextParams.n_ubatch = contextParams.n_ctx;
    contextParams.n_seq_max = 1;
    contextParams.embeddings = true;
    contextParams.pooling_type = llama_pooling_type::LLAMA_POOLING_TYPE_UNSPECIFIED;
    contextParams.no_perf = true;

    mEmbeddingContext.reset(llama_init_from_model(mEmbeddingModel.get(), contextParams));
    if(mEmbeddingContext == nullptr)
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "Failed to initialize RAG embedding model context.");
        return juce::Result::fail(juce::translate("Failed to initialize RAG embedding model context."));
    }

    if(llama_model_n_embd_out(mEmbeddingModel.get()) <= 0)
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG embedding model embedding size is invalid.");
        return juce::Result::fail(juce::translate("RAG embedding model embedding size is invalid."));
    }

    // The encoder-only models and the models without context memory (BERT-like models, that llama_decode would
    // forward to the encoder) are encoded, the other models are decoded
    auto const usesEncoder = [](llama_model const* model, llama_context const* context)
    {
        return llama_model_has_encoder(model) || llama_get_memory(context) == nullptr;
    };
    mEmbeddingUsesEncoder = usesEncoder(mEmbeddingModel.get(), mEmbeddingContext.get());

    contextParams.embeddings = true;
    contextParams.pooling_type = llama_pooling_type::LLAMA_POOLING_TYPE_RANK;
    mRerankerModel.reset(llama_model_load_from_file(rerankerModelFile.getFullPathName().toRawUTF8(), modelParams));
    if(mRerankerModel == nullptr)
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
        return juce::Result::fail(juce::translate("Failed to load RAG reranker model: FLNAME").replace("FLNAME", rerankerModelFile.getFullPathName()));
    }

    if(llama_model_has_encoder(mRerankerModel.get()) && llama_model_has_decoder(mRerankerModel.get()))
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "RAG reranker model with an encoder and a decoder is not supported.");
        return juce::Result::fail(juce::translate("RAG reranker model with an encoder and a decoder is not supported."));
    }
    contextParams.embeddings = false;
    mRerankerContext.reset(llama_init_from_model(mRerankerModel.get(), contextParams));
    if(mRerankerContext == nullptr)
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "Failed to initialize RAG reranker model context.");
        return juce::Result::fail(juce::translate("Failed to initialize RAG reranker model context."));
    }
    mRerankerUsesEncoder = usesEncoder(mRerankerModel.get(), mRerankerContext.get());

    if(llama_pooling_type(mRerankerContext.get()) != LLAMA_POOLING_TYPE_RANK)
    {
        releaseModels();
        MiscDebug("Application::Neuralyzer::Rag::Engine", "The context does not use RANK pooling. Verify that the GGUF actually contains cls.output.weight.");
        return juce::Result::fail(juce::translate("The context does not use RANK pooling."));
    }
    llama_set_embeddings(mRerankerContext.get(), true);

    return juce::Result::ok();
}

std::vector<float> Application::Neuralyzer::Rag::Engine::computeEmbedding(juce::String const& text)
{
    std::lock_guard<std::mutex> lock(mMutex);
    return computeEmbeddingUnlocked(text);
}

std::vector<float> Application::Neuralyzer::Rag::Engine::computeEmbeddingUnlocked(juce::String const& text)
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
    // The special tokens are added around the text but not parsed in the text (a text containing </s> must not end the sequence)
    auto numTokens = llama_tokenize(vocab, utf8Text, static_cast<int32_t>(utf8Size), nullptr, 0, true, false);
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
    auto const tokenizedCount = llama_tokenize(vocab, utf8Text, static_cast<int32_t>(utf8Size), tokens.data(), numTokens, true, false);
    MiscWeakAssert(tokenizedCount > 0);
    if(tokenizedCount <= 0)
    {
        return {};
    }
    tokens.resize(static_cast<size_t>(tokenizedCount));

    auto* context = mEmbeddingContext.get();
    if(auto* memory = llama_get_memory(context))
    {
        llama_memory_clear(memory, true);
    }
    // The text is processed in a single micro-batch, because encoding it by chunks would only keep the embedding of the last chunk
    auto const maxNumTokens = static_cast<size_t>(llama_n_ubatch(context));
    if(maxNumTokens == 0_z)
    {
        return {};
    }
    if(tokens.size() > maxNumTokens)
    {
        // Preserve the last (special) token
        auto const lastToken = tokens.back();
        tokens.resize(maxNumTokens);
        tokens.back() = lastToken;
    }

    auto const batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    auto const status = mEmbeddingUsesEncoder ? llama_encode(context, batch) : llama_decode(context, batch);
    MiscWeakAssert(status == 0);
    if(status != 0)
    {
        return {};
    }
    llama_synchronize(context);

    auto const* embeddingData = [&]()
    {
        auto const* data = llama_get_embeddings_seq(context, 0);
        return data != nullptr ? data : llama_get_embeddings_ith(context, static_cast<int32_t>(tokens.size()) - 1);
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
    std::lock_guard<std::mutex> lock(mMutex);
    return computeRerankerScoreUnlocked(query, resourceContent);
}

float Application::Neuralyzer::Rag::Engine::computeRerankerScoreUnlocked(juce::String const& query, juce::String const& resourceContent)
{
    MiscWeakAssert(mRerankerModel != nullptr && mRerankerContext != nullptr);
    if(mRerankerModel == nullptr || mRerankerContext == nullptr)
    {
        return 0.0f;
    }
    auto* context = mRerankerContext.get();
    auto* model = mRerankerModel.get();
    auto const* vocab = llama_model_get_vocab(model);

    auto tokens = [&]()
    {
        // Matches llama.cpp's embedding CLI (add_special = true), unlike llama.cpp's server which manages special tokens itself.
        // The special tokens are only parsed in the template and the separators, not in the query and the document.
        auto const queryText = query.toStdString();
        auto const documentText = resourceContent.toStdString();
        auto const* rerankPrompt = llama_model_chat_template(model, "rerank");
        if(rerankPrompt != nullptr)
        {
            static std::string const queryKey = "{query}";
            static std::string const documentKey = "{document}";
            std::string const prompt = rerankPrompt;
            std::vector<TextSegment> segments;
            auto position = 0_z;
            while(position < prompt.size())
            {
                auto const queryPosition = prompt.find(queryKey, position);
                auto const documentPosition = prompt.find(documentKey, position);
                auto const keyPosition = std::min(queryPosition, documentPosition);
                if(keyPosition == std::string::npos)
                {
                    segments.push_back({prompt.substr(position), true});
                    break;
                }
                auto const isQuery = keyPosition == queryPosition;
                segments.push_back({prompt.substr(position, keyPosition - position), true});
                segments.push_back({isQuery ? queryText : documentText, false});
                position = keyPosition + (isQuery ? queryKey.size() : documentKey.size());
            }
            return tokenizeSegments(vocab, segments);
        }
        std::string const addedSepToken = llama_vocab_get_add_sep(vocab) ? llama_vocab_get_text(vocab, llama_vocab_sep(vocab)) : "";
        std::string const addedEosToken = llama_vocab_get_add_eos(vocab) ? llama_vocab_get_text(vocab, llama_vocab_eos(vocab)) : "";
        return tokenizeSegments(vocab, {{queryText, false}, {addedEosToken + addedSepToken, true}, {documentText, false}});
    }();
    MiscWeakAssert(!tokens.empty());
    MiscWeakAssert(tokens.empty() || tokens.back() == llama_vocab_sep(vocab) || tokens.back() == llama_vocab_eos(vocab));

#if JUCE_DEBUG
    MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Query: ") + query + juce::String(" - Resource: ") + resourceContent);
    juce::String prompt;
    for(auto const& token : tokens)
    {
        prompt += common_token_to_piece(context, token).c_str();
    }
    MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Tokens: ") + prompt);
#endif

    // The reranker is non-causal: all the tokens must fit in a single micro-batch (llama.cpp aborts otherwise)
    auto const maxNumTokens = static_cast<size_t>(llama_n_ubatch(context));
    if(tokens.empty() || maxNumTokens == 0_z)
    {
        return 0.0f;
    }
    if(tokens.size() > maxNumTokens)
    {
        // Preserve the last (special) token
        auto const lastToken = tokens.back();
        tokens.resize(maxNumTokens);
        tokens.back() = lastToken;
    }

    common_batch batch(context);
    for(auto j = 0_z; j < tokens.size(); ++j)
    {
        batch.add(tokens[j], static_cast<llama_pos>(j), 0, true);
    }

    // Clear the memory cache before each new pair (no shared context between pairs)
    llama_memory_clear(llama_get_memory(context), true);

    if(llama_process(context, mRerankerUsesEncoder ? LLAMA_PROCESS_TYPE_ENCODE : LLAMA_PROCESS_TYPE_DECODE, batch.get()) != 0)
    {
        MiscWeakAssert(false);
        return 0.0f;
    }

    float score = -1e6;
    for(auto i = 0_z; i < batch.tokens.size(); ++i)
    {
        if(batch.tokens[i].output)
        {
            auto const* embd = llama_get_embeddings_seq(context, batch.tokens[i].seq_id);
            MiscWeakAssert(embd != nullptr);
            if(embd == nullptr)
            {
                score = -1e6;
                continue;
            }
            score = embd[0];
        }
    }
    auto const relevance = 1.0f / (1.0f + std::exp(-score));
    MiscDebug("Application::Neuralyzer::Rag::Engine", juce::String("Score: ") + juce::String(score) + " - Relevance: " + juce::String(relevance));
    return relevance;
}

juce::Result Application::Neuralyzer::Rag::Engine::setResources(std::vector<Resource> const& resources)
{
    std::lock_guard<std::mutex> lock(mMutex);
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

    // The identifiers are unique in each document but two documents could still produce the same identifier
    std::set<juce::String> ids;
    for(auto const& chunk : resources)
    {
        if(mShouldQuit.load())
        {
            mIndexedEntries.clear();
            return juce::Result::fail(juce::translate("The indexation of the documentation has been aborted."));
        }
        auto const embedding = computeEmbeddingUnlocked(getTextWithContext(chunk));
        if(!embedding.empty())
        {
            matrix.insert(matrix.end(), embedding.begin(), embedding.end());
            mIndexedEntries.push_back(chunk);
            auto& entry = mIndexedEntries.back();
            for(auto suffix = 2; !ids.insert(entry.id).second; ++suffix)
            {
                entry.id = chunk.id + "-" + juce::String(suffix);
            }
        }
    }
    if(mIndexedEntries.empty())
    {
        return juce::Result::fail(juce::translate("No valid resource entries to index."));
    }

    mSpace = std::make_unique<hnswlib::InnerProductSpace>(numEmbd);
    mIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(mSpace.get(), mIndexedEntries.size(), 16, 100);
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

    auto const queryEmbedding = computeEmbeddingUnlocked(trimmedPrompt);
    if(queryEmbedding.empty())
    {
        return {};
    }

    auto const topK = std::min(30_z, mIndexedEntries.size());
    auto matches = mIndex->searchKnn(queryEmbedding.data(), topK);
    std::vector<std::pair<float, size_t>> ranked;
    ranked.reserve(topK);
    while(!matches.empty())
    {
        ranked.push_back(matches.top());
        matches.pop();
    }
    std::sort(ranked.begin(), ranked.end());

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

    // The labels are the indices of the entries, which are stored in the order of the documents
    auto const isSameSection = [this](size_t lhs, size_t rhs)
    {
        auto const& lhsResource = mIndexedEntries.at(lhs);
        auto const& rhsResource = mIndexedEntries.at(rhs);
        return lhsResource.document == rhsResource.document && lhsResource.section == rhsResource.section;
    };

    for(auto const& [distance, label] : ranked)
    {
        if(distance > maxDistance)
        {
            break;
        }
        addResource(label);
        // Add the adjacent chunks of the same section so that the reranker can also consider the continuation of the passage
        if(label > 0_z && isSameSection(label, label - 1_z))
        {
            addResource(label - 1_z);
        }
        if(label + 1_z < mIndexedEntries.size() && isSameSection(label, label + 1_z))
        {
            addResource(label + 1_z);
        }
    }

    // Re-rank resources using the reranker model if available
    if(mRerankerModel != nullptr && mRerankerContext != nullptr)
    {
        for(auto& resource : resources)
        {
            resource.score = computeRerankerScoreUnlocked(trimmedPrompt, getTextWithContext(resource));
        }

        // Sort by reranker score (descending)
        std::sort(resources.begin(), resources.end(), [](auto const& lhs, auto const& rhs)
                  {
                      return lhs.score > rhs.score;
                  });

        // Keep only confident matches
        std::erase_if(resources, [&](auto const& resource)
                      {
                          return resource.score < minScore;
                      });
    }
    if(resources.size() > maxResources)
    {
        resources.resize(maxResources);
    }
    return resources;
}

std::vector<Application::Neuralyzer::Rag::Resource> Application::Neuralyzer::Rag::Engine::getResources(std::vector<juce::String> const& ids) const
{
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<Resource> resources;
    resources.reserve(ids.size());
    std::copy_if(mIndexedEntries.cbegin(), mIndexedEntries.cend(), std::back_inserter(resources), [&](auto const& resource)
                 {
                     return std::find(ids.cbegin(), ids.cend(), resource.id) != ids.cend();
                 });
    return resources;
}

juce::Result Application::Neuralyzer::Rag::Engine::checkAvailability() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    if(mEmbeddingModel == nullptr || mRerankerModel == nullptr)
    {
        return juce::Result::fail("The documentation search is not available because the RAG models are not installed. The user can download the RAG models from the agent settings of Partiels.");
    }
    if(mIndex == nullptr || mIndexedEntries.empty())
    {
        return juce::Result::fail("The documentation search is not available because the documentation is not indexed (the indexation failed or was aborted).");
    }
    return juce::Result::ok();
}

nlohmann::json Application::Neuralyzer::Rag::Engine::searchDocs(std::string const& query, size_t maxNumResources, float minScore, float maxDistance)
{
    auto const availability = checkAvailability();
    if(availability.failed())
    {
        return createTextResponse(availability.getErrorMessage().toStdString(), true);
    }

    auto const resources = computeResources(juce::String::fromUTF8(query.c_str()), maxNumResources, minScore, maxDistance);
    std::string message;
    if(resources.empty())
    {
        message += "No relevant document was found.";
    }
    else
    {
        message += "Relevant documents:\n";
        for(auto index = 0_z; index < resources.size(); ++index)
        {
            auto const& resource = resources.at(index);
            message += "\n";
            message += "[" + std::to_string(index + 1) + "]" + "\n";
            message += "ID: " + resource.id.toStdString() + "\n";
            message += "Title: " + resource.document.toStdString() + "\n";
            message += "Section: " + resource.section.toStdString() + "\n";
            message += "Score: " + std::to_string(resource.score) + "\n";
        }
        message += "\nRetrieve the contents of the documentation using the load_docs tool with the IDs of the documents.";
    }
    return createTextResponse(message, false);
}

nlohmann::json Application::Neuralyzer::Rag::Engine::loadDocs(std::vector<std::string> const& ids)
{
    auto const availability = checkAvailability();
    if(availability.failed())
    {
        return createTextResponse(availability.getErrorMessage().toStdString(), true);
    }

    auto const resources = getResources(std::vector<juce::String>{ids.cbegin(), ids.cend()});
    if(resources.empty())
    {
        return createTextResponse("No document was found for the given ids.", true);
    }

    std::string message;
    message += "Relevant documents:\n";
    for(auto const& resource : resources)
    {
        message += "\n--------\n";
        message += "ID: " + resource.id.toStdString() + "\n";
        message += "Title: " + resource.document.toStdString() + "\n";
        message += "Section: " + resource.section.toStdString() + "\n";
        message += "Content:\n" + resource.content.toStdString() + "\n";
    }
    message += "\nSearch for complementary documentation using the search_docs tool.";
    return createTextResponse(message, false);
}

void Application::Neuralyzer::Rag::makeUniqueIdentifiers(std::vector<Resource>& resources)
{
    std::set<juce::String> ids;
    for(auto& resource : resources)
    {
        auto const id = resource.id;
        for(auto suffix = 2; !ids.insert(resource.id).second; ++suffix)
        {
            resource.id = id + "-" + juce::String(suffix);
        }
    }
}

std::vector<std::pair<juce::String, juce::String>> Application::Neuralyzer::Rag::getContextDocuments(juce::File const& directory)
{
    auto files = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.md");
    files.sort();
    std::vector<std::pair<juce::String, juce::String>> documents;
    for(auto const& file : files)
    {
        auto const text = file.loadFileAsString();
        if(text.trim().isNotEmpty())
        {
            documents.emplace_back(file.getRelativePathFrom(directory).upToLastOccurrenceOf(".", false, false), text);
        }
    }
    return documents;
}

void Application::Neuralyzer::Rag::Engine::setShouldQuit(bool state)
{
    mShouldQuit.store(state);
}

class RagTest
: public juce::UnitTest
{
public:
    RagTest()
    : juce::UnitTest("Neuralyzer", "Application")
    {
    }

    ~RagTest() override = default;

    void runTest() override
    {
        testCreateResources();
        testContextDocuments();
        testResourceSerialization();
        testEngineWithoutModels();

        namespace Rag = Application::Neuralyzer::Rag;
        if(!Rag::getEmbeddingModelFile().existsAsFile() || !Rag::getRerankerModelFile().existsAsFile())
        {
            logMessage("The RAG models are not available: the tests of the models are skipped.");
            return;
        }
        Rag::Engine engine;
        if(!testModels(engine))
        {
            return;
        }
        testSearch(engine);
        testDocumentation(engine);
    }

private:
    using Resource = Application::Neuralyzer::Rag::Resource;

    static Application::Neuralyzer::Ast::Section createSection(std::vector<juce::String> hierarchy, juce::String const& content)
    {
        Application::Neuralyzer::Ast::Section section;
        section.hierarchy = std::move(hierarchy);
        section.content = content;
        return section;
    }

    static juce::StringArray getIds(std::vector<Resource> const& resources)
    {
        juce::StringArray ids;
        for(auto const& resource : resources)
        {
            ids.add(resource.id);
        }
        return ids;
    }

    static juce::String getText(nlohmann::json const& response)
    {
        if(!response.contains("content") || !response.at("content").is_array() || response.at("content").empty())
        {
            return {};
        }
        return juce::String::fromUTF8(response.at("content").at(0).value("text", std::string{}).c_str());
    }

    void testCreateResources()
    {
        namespace Rag = Application::Neuralyzer::Rag;
        namespace Ast = Application::Neuralyzer::Ast;
        beginTest("Create Resources");

        juce::StringArray words;
        for(auto i = 0; i < 100; ++i)
        {
            words.add("word" + juce::String(i));
        }
        auto const createDocument = [&](juce::String const& name)
        {
            Ast::Document document;
            document.document = name;
            document.sections.push_back(createSection({"Title", "Section"}, "First line\n" + words.joinIntoString(" ") + "\nLast line"));
            return document;
        };
        static auto constexpr maxResourceLength = 64;
        auto const resources = Rag::createResources(createDocument("Document"), maxResourceLength);

        testCase("Split Long Lines", [&]()
                 {
                     expect(resources.size() > 1_z);
                     for(auto const& resource : resources)
                     {
                         expect(resource.content.isNotEmpty());
                         expect(resource.content.length() <= maxResourceLength, resource.content);
                         expectEquals(resource.document, juce::String("Document"));
                         expectEquals(resource.section, juce::String("Title > Section"));
                     }
                 });
        testCase("Keep All Content", [&]()
                 {
                     juce::StringArray contents;
                     for(auto const& resource : resources)
                     {
                         contents.add(resource.content);
                     }
                     auto const expected = juce::StringArray::fromTokens("First line " + words.joinIntoString(" ") + " Last line", " ", "");
                     auto const result = juce::StringArray::fromTokens(contents.joinIntoString("\n"), " \n", "");
                     expect(result == expected, result.joinIntoString(" "));
                 });
        testCase("Unique Identifiers", [&]()
                 {
                     expectEquals(resources.front().id, juce::String("document-section-1"));
                     auto allResources = resources;
                     auto const otherResources = Rag::createResources(createDocument("Other Document"), maxResourceLength);
                     allResources.insert(allResources.end(), otherResources.cbegin(), otherResources.cend());
                     std::set<juce::String> ids;
                     for(auto const& resource : allResources)
                     {
                         ids.insert(resource.id);
                     }
                     expectEquals(ids.size(), allResources.size());
                 });
        testCase("Short Identifiers", [&]()
                 {
                     Ast::Document guide;
                     guide.document = "My Guide";
                     // The title of the document is ignored
                     guide.sections.push_back(createSection({"My Guide"}, "Introduction"));
                     // The last heading is not unique: all the headings are used
                     guide.sections.push_back(createSection({"My Guide", "Track", "Preset"}, "Track preset"));
                     guide.sections.push_back(createSection({"My Guide", "Group", "Preset"}, "Group preset"));
                     // All the headings are not unique: a suffix is added
                     guide.sections.push_back(createSection({"My Guide", "Export"}, "First export"));
                     guide.sections.push_back(createSection({"My Guide", "Export"}, "Second export"));
                     expectEquals(getIds(Rag::createResources(guide, 1200)).joinIntoString(" "), juce::String("my-guide-1 my-guide-track-preset-1 my-guide-group-preset-1 my-guide-export-1 my-guide-export-2-1"));

                     // The title is compared without punctuation and only the last heading is used
                     Ast::Document tutorial;
                     tutorial.document = "How to do it";
                     tutorial.sections.push_back(createSection({"How to do it?", "Parameters", "Window Size"}, "Content"));
                     expectEquals(getIds(Rag::createResources(tutorial, 1200)).joinIntoString(" "), juce::String("how-to-do-it-window-size-1"));
                 });
        testCase("Oversized Words", [&]()
                 {
                     // A word that exceeds the maximum length is kept whole in its own resource
                     auto const longWord = juce::String::repeatedString("a", maxResourceLength * 2);
                     Ast::Document document;
                     document.document = "Document";
                     document.sections.push_back(createSection({"Section"}, "before " + longWord + " after"));
                     auto const result = Rag::createResources(document, maxResourceLength);
                     expectEquals(result.size(), 3_z);
                     if(result.size() == 3_z)
                     {
                         expectEquals(result.at(0).content, juce::String("before"));
                         expectEquals(result.at(1).content, longWord);
                         expectEquals(result.at(2).content, juce::String("after"));
                     }
                 });
        testCase("Sections", [&]()
                 {
                     Ast::Document document;
                     document.document = "Manual";
                     document.sections.push_back(createSection({}, "Introduction"));
                     document.sections.push_back(createSection({"Chapter"}, "\n\n"));
                     document.sections.push_back(createSection({"Chapter", "Part"}, "First line\r\nSecond line\r\n"));
                     document.sections.push_back(createSection({"Appendix"}, "Notes"));
                     auto const result = Rag::createResources(document, 1200);
                     expectEquals(getIds(result).joinIntoString(" "), juce::String("manual-1 manual-part-1 manual-appendix-1"));
                     if(result.size() == 3_z)
                     {
                         // The content before the first heading has no section
                         expectEquals(result.at(0).section, juce::String());
                         // The sections without content are ignored and the carriage returns are removed
                         expectEquals(result.at(1).section, juce::String("Chapter > Part"));
                         expectEquals(result.at(1).content, juce::String("First line\nSecond line"));
                     }
                 });
    }

    void testContextDocuments()
    {
        namespace Rag = Application::Neuralyzer::Rag;
        namespace Ast = Application::Neuralyzer::Ast;
        beginTest("Context Documents");

        juce::TemporaryFile const temporaryDirectory;
        auto const directory = temporaryDirectory.getFile();
        expect(directory.createDirectory().wasOk());
        expect(directory.getChildFile("Notes.md").replaceWithText("# Notes\nMy notes"));
        expect(directory.getChildFile("Empty.md").replaceWithText(" \n"));
        expect(directory.getChildFile("Ignored.txt").replaceWithText("# Ignored\nText"));
        expect(directory.getChildFile("Folder").createDirectory().wasOk());
        expect(directory.getChildFile("Folder").getChildFile("Guide.md").replaceWithText("# Guide\nMy guide", false, false, "\n"));

        testCase("Markdown Files", [&]()
                 {
                     auto const documents = Rag::getContextDocuments(directory);
                     expectEquals(documents.size(), 2_z);
                     if(documents.size() == 2_z)
                     {
                         expectEquals(documents.at(0_z).first, juce::File::addTrailingSeparator("Folder") + "Guide");
                         expectEquals(documents.at(0_z).second, juce::String("# Guide\nMy guide"));
                         expectEquals(documents.at(1_z).first, juce::String("Notes"));
                     }
                     expect(Rag::getContextDocuments(directory.getChildFile("Missing")).empty());
                 });
        testCase("Unique Identifiers", [&]()
                 {
                     // A context file can have the name of another document
                     auto const document = Ast::parseMarkdown("# Notes\nMy notes\n## Section\nText", "Notes");
                     auto resources = Rag::createResources(document, 1200);
                     auto const copy = resources;
                     resources.insert(resources.end(), copy.cbegin(), copy.cend());
                     Rag::makeUniqueIdentifiers(resources);
                     std::set<juce::String> ids;
                     for(auto const& resource : resources)
                     {
                         ids.insert(resource.id);
                     }
                     expectEquals(ids.size(), resources.size());
                     expectEquals(resources.front().id, copy.front().id);
                     expectEquals(resources.at(copy.size()).id, copy.front().id + "-2");
                 });
        directory.deleteRecursively();
    }

    void testResourceSerialization()
    {
        namespace Rag = Application::Neuralyzer::Rag;
        beginTest("Resource Serialization");

        Rag::Resource resource;
        resource.id = "guide-section-1";
        resource.content = "Content";
        resource.section = "Guide > Section";
        resource.document = "Guide";
        resource.score = 0.5f;

        testCase("Round Trip", [&]()
                 {
                     nlohmann::json const json = resource;
                     auto const copy = json.get<Rag::Resource>();
                     expect(copy == resource);
                     expectEquals(copy.score, resource.score);
                 });
        testCase("Missing Fields", [&]()
                 {
                     auto const partial = nlohmann::json{{"id", " guide-section-2 "}}.get<Rag::Resource>();
                     expectEquals(partial.id, juce::String("guide-section-2"));
                     expect(partial.content.isEmpty() && partial.section.isEmpty() && partial.document.isEmpty());
                     expectEquals(partial.score, 0.0f);
                 });
        testCase("Comparison", [&]()
                 {
                     // The score is not part of the identity of a resource
                     auto scored = resource;
                     scored.score = 0.1f;
                     expect(scored == resource);
                     auto other = resource;
                     other.id = "guide-section-2";
                     expect(!(other == resource));
                     expect(resource < other && !(other < resource));
                 });
    }

    void testEngineWithoutModels()
    {
        namespace Rag = Application::Neuralyzer::Rag;
        beginTest("Engine Without Models");

        Rag::Engine engine;
        testCase("Missing Model Files", [&]()
                 {
                     auto const missingFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("missing", ".gguf");
                     auto const result = engine.initializeModels(missingFile, missingFile);
                     expect(result.failed());
                     expect(result.getErrorMessage().contains(missingFile.getFullPathName()), result.getErrorMessage());
                 });
        testCase("Search Without Index", [&]()
                 {
                     expect(engine.computeResources("How do I show a spectrogram?").empty());
                     expect(engine.getResources({"guide-section-1"}).empty());

                     auto const search = engine.searchDocs("How do I show a spectrogram?", 4_z);
                     expect(search.value("isError", false));
                     expect(getText(search).contains("RAG models are not installed"), getText(search));

                     auto const load = engine.loadDocs({"guide-section-1"});
                     expect(load.value("isError", false));
                     expect(getText(load).contains("RAG models are not installed"), getText(load));
                 });
    }

    bool testModels(Application::Neuralyzer::Rag::Engine& engine)
    {
        namespace Rag = Application::Neuralyzer::Rag;
        beginTest("Models");

        auto const initResult = engine.initializeModels(Rag::getEmbeddingModelFile(), Rag::getRerankerModelFile());
        expect(initResult.wasOk(), initResult.getErrorMessage());
        if(initResult.failed())
        {
            return false;
        }

        testCase("Special Tokens", [&]()
                 {
                     auto modelParams = llama_model_default_params();
                     modelParams.vocab_only = true;
                     llama_model_ptr const model(llama_model_load_from_file(Rag::getRerankerModelFile().getFullPathName().toRawUTF8(), modelParams));
                     expect(model != nullptr);
                     if(model == nullptr)
                     {
                         return;
                     }
                     auto const* vocab = llama_model_get_vocab(model.get());
                     auto const eos = llama_vocab_eos(vocab);
                     std::string const eosText = llama_vocab_get_text(vocab, eos);
                     // Without special text in the query and the document, the segments match the whole text
                     auto const reference = common_tokenize(vocab, "What is a spectrogram?" + eosText + "A spectrogram shows the frequencies.", true, true);
                     auto const tokens = tokenizeSegments(vocab, {{"What is a spectrogram?", false}, {eosText, true}, {"A spectrogram shows the frequencies.", false}});
                     expect(tokens == reference);
                     // The special text of a query or a document is not parsed as a special token
                     auto const escaped = tokenizeSegments(vocab, {{"What is " + eosText + "?", false}});
                     expectEquals(std::count(escaped.cbegin(), escaped.cend(), eos), static_cast<std::ptrdiff_t>(llama_vocab_get_add_eos(vocab) ? 1 : 0));
                 });
        testCase("Compute Embeddings", [&]()
                 {
                     auto const query = engine.computeEmbedding("What is the capital of France?");
                     auto const embedding1 = engine.computeEmbedding("The capital of France is Paris!");
                     auto const embedding2 = engine.computeEmbedding("The violin has four strings.");
                     expect(!query.empty() && query.size() == embedding1.size() && query.size() == embedding2.size());
                     if(query.empty() || query.size() != embedding1.size() || query.size() != embedding2.size())
                     {
                         return;
                     }
                     // The embeddings are normalized so the inner product is the cosine similarity
                     auto const norm = std::sqrt(std::inner_product(query.cbegin(), query.cend(), query.cbegin(), 0.0f));
                     expectWithinAbsoluteError(norm, 1.0f, 1e-3f);
                     auto const similarity1 = std::inner_product(query.cbegin(), query.cend(), embedding1.cbegin(), 0.0f);
                     auto const similarity2 = std::inner_product(query.cbegin(), query.cend(), embedding2.cbegin(), 0.0f);
                     expect(similarity1 > similarity2, juce::String(similarity1) + " <= " + juce::String(similarity2));
                     expect(engine.computeEmbedding({}).empty());
                 });
        testCase("Compute Scores", [&]()
                 {
                     auto const score1 = engine.computeRerankerScore("What is the capital of France?", "The capital of France is Paris!");
                     auto const score2 = engine.computeRerankerScore("What is the capital of France?", "Berlin is the capital of Germany.");
                     auto const score3 = engine.computeRerankerScore("What is the capital of France?", "hi");
                     expect(score1 > score2, juce::String(score1) + " <= " + juce::String(score2));
                     expect(score2 > score3, juce::String(score2) + " <= " + juce::String(score3));
                     for(auto const score : {score1, score2, score3})
                     {
                         expect(score >= 0.0f && score <= 1.0f, juce::String(score));
                     }
                 });
        testCase("Long Inputs", [&]()
                 {
                     // The inputs that exceed the micro-batch size are truncated (llama.cpp aborts otherwise)
                     auto const longText = juce::String::repeatedString("The spectrogram shows the frequencies of the sound over time. ", 300);
                     expect(!engine.computeEmbedding(longText).empty());
                     auto const longQueryScore = engine.computeRerankerScore(longText, "A spectrogram shows the frequencies.");
                     expect(longQueryScore >= 0.0f && longQueryScore <= 1.0f, juce::String(longQueryScore));
                     auto const longDocumentScore = engine.computeRerankerScore("What is a spectrogram?", longText);
                     expect(longDocumentScore >= 0.0f && longDocumentScore <= 1.0f, juce::String(longDocumentScore));
                 });
        testCase("Special Text In Inputs", [&]()
                 {
                     expect(!engine.computeEmbedding("What is </s> a spectrogram? <s>").empty());
                     auto const score = engine.computeRerankerScore("What is </s> a spectrogram?", "A spectrogram <s>shows</s> the frequencies.");
                     expect(score >= 0.0f && score <= 1.0f, juce::String(score));
                 });
        return true;
    }

    void testSearch(Application::Neuralyzer::Rag::Engine& engine)
    {
        namespace Rag = Application::Neuralyzer::Rag;
        namespace Ast = Application::Neuralyzer::Ast;
        beginTest("Search");

        static auto const query = juce::String("How do I show a spectrogram?");
        Ast::Document guide;
        guide.document = "Guide";
        guide.sections.push_back(createSection({"Guide", "Spectrogram"}, "To display a spectrogram, create a track with a Fourier transform plugin."));
        guide.sections.push_back(createSection({"Guide", "Tempo"}, "To estimate the tempo, create a track with a beat tracking plugin."));
        guide.sections.push_back(createSection({"Guide", "Export"}, "To export the results, use the export command of the file menu."));
        auto const guideResources = Rag::createResources(guide, 1200);
        auto const indexGuide = [&]()
        {
            auto const result = engine.setResources(guideResources);
            expect(result.wasOk(), result.getErrorMessage());
        };

        testCase("Relevant Resource First", [&]()
                 {
                     indexGuide();
                     auto const results = engine.computeResources(query);
                     expect(!results.empty());
                     if(!results.empty())
                     {
                         expectEquals(results.front().section, juce::String("Guide > Spectrogram"));
                     }
                     for(size_t i = 1_z; i < results.size(); ++i)
                     {
                         expect(results.at(i - 1_z).score >= results.at(i).score);
                     }
                 });
        testCase("Limits", [&]()
                 {
                     indexGuide();
                     expectEquals(engine.computeResources(query, 1_z, 0.0f, 2.0f).size(), 1_z);
                     expectEquals(engine.computeResources(query, 10_z, 0.0f, 2.0f).size(), guideResources.size());
                     // The relevance is below 1 and the embedding distance is above 0
                     expect(engine.computeResources(query, 10_z, 1.1f, 2.0f).empty());
                     expect(engine.computeResources(query, 10_z, 0.0f, -1.0f).empty());
                     expect(engine.computeResources("   ").empty());
                 });
        testCase("Search And Load Docs", [&]()
                 {
                     indexGuide();
                     auto const search = engine.searchDocs(query.toStdString(), 4_z);
                     expect(!search.value("isError", true));
                     juce::StringArray ids;
                     for(auto const& line : juce::StringArray::fromLines(getText(search)))
                     {
                         if(line.startsWith("ID: "))
                         {
                             ids.add(line.fromFirstOccurrenceOf("ID: ", false, false).trim());
                         }
                     }
                     expect(ids.contains("guide-spectrogram-1"), ids.joinIntoString(", "));

                     std::vector<std::string> idList;
                     for(auto const& id : ids)
                     {
                         idList.push_back(id.toStdString());
                     }
                     auto const load = engine.loadDocs(idList);
                     expect(!load.value("isError", true));
                     expect(getText(load).contains("Fourier transform plugin"), getText(load));
                     expectEquals(engine.getResources({"guide-spectrogram-1"}).size(), 1_z);
                 });
        testCase("Adjacent Chunks", [&]()
                 {
                     // Three chunks of the same section: only the first one is relevant
                     Ast::Document notes;
                     notes.document = "Notes";
                     notes.sections.push_back(createSection({"Section"}, "The window size of the spectrogram defines the number of samples used to compute each Fourier transform.\n"
                                                                         "The weather was sunny and the children played in the garden all afternoon.\n"
                                                                         "The recipe requires two eggs, flour, sugar and a pinch of salt."));
                     auto const resources = Rag::createResources(notes, 120);
                     expectEquals(getIds(resources).joinIntoString(" "), juce::String("notes-section-1 notes-section-2 notes-section-3"));
                     if(resources.size() != 3_z)
                     {
                         return;
                     }
                     expect(engine.setResources(resources).wasOk());

                     // The embedding distances are computed like the index (see setResources)
                     static auto const notesQuery = juce::String("What does the window size of a spectrogram define?");
                     auto const queryEmbedding = engine.computeEmbedding(notesQuery);
                     auto const getDistance = [&](Resource const& resource)
                     {
                         auto const embedding = engine.computeEmbedding(getTextWithContext(resource));
                         return 1.0f - std::inner_product(queryEmbedding.cbegin(), queryEmbedding.cend(), embedding.cbegin(), 0.0f);
                     };
                     auto const relevantDistance = getDistance(resources.at(0));
                     auto const otherDistance = std::min(getDistance(resources.at(1)), getDistance(resources.at(2)));
                     expect(relevantDistance < otherDistance, juce::String(relevantDistance) + " >= " + juce::String(otherDistance));

                     // Only the first chunk is close enough, its neighbor is added but not the neighbor of its neighbor
                     auto const maxDistance = (relevantDistance + otherDistance) / 2.0f;
                     auto const ids = getIds(engine.computeResources(notesQuery, 10_z, 0.0f, maxDistance));
                     expect(ids.contains("notes-section-1") && ids.contains("notes-section-2") && !ids.contains("notes-section-3"), ids.joinIntoString(", "));
                 });
        testCase("Unique Identifiers In Index", [&]()
                 {
                     // Two documents can produce the same identifier (for instance "a" with "b-c" and "a-b" with "c")
                     Rag::Resource first;
                     first.id = "a-b-c-1";
                     first.document = "A";
                     first.section = "B C";
                     first.content = "To display a spectrogram, create a track with a Fourier transform plugin.";
                     auto second = first;
                     second.document = "A B";
                     second.section = "C";
                     second.content = "To estimate the tempo, create a track with a beat tracking plugin.";
                     expect(engine.setResources({first, second}).wasOk());
                     expectEquals(engine.getResources({"a-b-c-1"}).size(), 1_z);
                     auto const renamed = engine.getResources({"a-b-c-1-2"});
                     expect(renamed.size() == 1_z && renamed.front().document == "A B");
                 });
        testCase("Concurrent Searches", [&]()
                 {
                     indexGuide();
                     auto const reference = getIds(engine.computeResources(query));
                     std::array<juce::StringArray, 4_z> results;
                     std::vector<std::thread> threads;
                     for(auto& result : results)
                     {
                         threads.emplace_back([&]()
                                              {
                                                  result = getIds(engine.computeResources(query));
                                              });
                     }
                     for(auto& thread : threads)
                     {
                         thread.join();
                     }
                     for(auto const& result : results)
                     {
                         expect(result == reference, result.joinIntoString(", ") + " != " + reference.joinIntoString(", "));
                     }
                 });
        testCase("Reload Models", [&]()
                 {
                     indexGuide();
                     // The models are reloaded on the same engine (the previous index is released)
                     auto const reloadResult = engine.initializeModels(Rag::getEmbeddingModelFile(), Rag::getRerankerModelFile());
                     expect(reloadResult.wasOk(), reloadResult.getErrorMessage());
                     expect(engine.computeResources(query).empty());
                     indexGuide();
                     auto const results = engine.computeResources(query);
                     expect(!results.empty() && results.front().section == "Guide > Spectrogram");
                 });
    }

    void testDocumentation(Application::Neuralyzer::Rag::Engine& engine)
    {
        namespace Rag = Application::Neuralyzer::Rag;
        namespace Ast = Application::Neuralyzer::Ast;
        beginTest("Documentation");

        // The manual and the tutorials are indexed like in the application (see the MCP resources and BackgroundAgent),
        // the list of the installed plugins is ignored because it requires the plugin scanner of the application
        std::vector<Resource> resources;
        auto const addDocument = [&](juce::String const& text, juce::String const& title)
        {
            auto const documentResources = Rag::createResources(Ast::parseMarkdown(text, title, false), 1200);
            resources.insert(resources.end(), documentResources.cbegin(), documentResources.cend());
        };
        // The MCP resource of the manual adds a title to the content of the manual
        addDocument("# Partiels User Manual\n" + juce::String::fromUTF8(AnlNeuralyzerData::PartielsManual_md, AnlNeuralyzerData::PartielsManual_mdSize), "Partiels User Manual");
        for(auto i = 0; i < AnlNeuralyzerTutorialData::namedResourceListSize; ++i)
        {
            auto size = 0;
            auto const* data = AnlNeuralyzerTutorialData::getNamedResource(AnlNeuralyzerTutorialData::namedResourceList[i], size);
            addDocument(juce::String::fromUTF8(data, size), juce::File::createFileWithoutCheckingPath(AnlNeuralyzerTutorialData::originalFilenames[i]).getFileNameWithoutExtension());
        }
        expect(!resources.empty());
        auto const indexResult = engine.setResources(resources);
        expect(indexResult.wasOk(), indexResult.getErrorMessage());
        if(indexResult.failed())
        {
            return;
        }

        // clang-format off
        // The query, the expected document and a part of the expected section
        std::vector<std::tuple<juce::String, juce::String, juce::String>> const expectations
        {
              {"How do I change the window size of the FFT of a spectrogram?", "How to configure a Fourier transform spectrogram", "Window Size"}
            , {"How can I transcribe the words spoken in an audio file?", "Analysis Workflows", "Analyze the text of the speech"}
            , {"How do I improve the visualization of a group of tracks?", "Graphical Workflows", "Improve the visualization of a group"}
            , {"How do I export the analysis results to a CSV file?", "Partiels User Manual", "7. Export analyses"}
              // The content of the expected section does not mention the color of a track: the section is required by the reranker
            , {"How do I change the color of a track?", "Partiels User Manual", "9.2.1. Colors"}
        };
        // clang-format on
        for(auto const& [query, document, section] : expectations)
        {
            auto const results = engine.computeResources(query);
            juce::StringArray documents;
            for(auto const& result : results)
            {
                documents.add(result.document + " > " + result.section + " (" + juce::String(result.score, 3) + ")");
            }
            logMessage(query + "\n  " + documents.joinIntoString("\n  "));
            auto const found = std::any_of(results.cbegin(), results.cend(), [&expectedDocument = document, &expectedSection = section](auto const& result)
                                           {
                                               return result.document == expectedDocument && result.section.contains(expectedSection);
                                           });
            expect(found, query + " -> " + documents.joinIntoString(", "));
        }
    }
};

static RagTest ragTest;

ANALYSE_FILE_END
