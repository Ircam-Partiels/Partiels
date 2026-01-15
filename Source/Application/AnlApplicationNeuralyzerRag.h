#pragma once

#include "AnlApplicationNeuralyzerAst.h"
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4127 4245 4267 4305 4457 4505 4701)
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wunused-function", "-Wextra-semi", "-Wshadow", "-Wsign-conversion", "-Wshorten-64-to-32", "-Wimplicit-int-float-conversion", "-Wimplicit-int-conversion", "-Wconditional-uninitialized", "-Wcast-align", "-Winconsistent-missing-destructor-override")
#include <hnswlib/hnswlib.h>
#include <llama-cpp.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Rag
        {
            //! @brief Gets the directory where the RAG models are stored
            juce::File getDefaultModelDirectory();
            //! @brief Gets the directory of the Markdown files (.md) added by the user to the indexed documentation
            juce::File getContextDirectory();
            //! @brief Gets the URL used to download the embedding model
            juce::URL getEmbeddingModelUrl();
            //! @brief Gets the URL used to download the reranker model
            juce::URL getRerankerModelUrl();
            //! @brief Gets the local file of the embedding model
            juce::File getEmbeddingModelFile();
            //! @brief Gets the local file of the reranker model
            juce::File getRerankerModelFile();

            //! @brief Downloads the embedding and reranker models if they are not available locally
            //! @details The user is notified once both models have been downloaded.
            void downloadModelsIfNecessary();

            //! @brief Asks the user whether the embedding and reranker models should be downloaded
            //! @details The user is asked if the user has not decided yet and if the models are not available locally.
            //! If the user has accepted, the models are downloaded if necessary. The choice is saved in the accessor of
            //! the agent.
            //! @param callback The function called once the user has answered (or immediately if the user is not asked)
            void askToDownloadModels(std::function<void()> callback);

            //! @brief A chunk of documentation that can be indexed and retrieved
            struct Resource
            {
                juce::String id;       //!< The unique identifier of the chunk
                juce::String content;  //!< The text of the chunk
                juce::String section;  //!< The hierarchy of the section (separated by " > ")
                juce::String document; //!< The name of the document
                float score = 0.0f;    //!< The relevance score (only set by the search)

                bool operator==(Resource const& rhs) const noexcept;
                bool operator<(Resource const& rhs) const;

                friend void to_json(nlohmann::json& json, Resource const& resource);
                friend void from_json(nlohmann::json const& json, Resource& resource);
            };

            //! @brief Splits the sections of a document into resources
            //! @param document The parsed document
            //! @param maxResourceLength The maximum number of characters of a resource (a single word can exceed it)
            //! @return The resources in the order of the document
            std::vector<Resource> createResources(Ast::Document const& document, int maxResourceLength);

            //! @brief Makes the identifiers of resources unique by appending a numeric suffix to the duplicates
            //! @details The identifiers are unique in a document but the identifiers of two documents can be the same
            //! (for instance, a context file with the name of a document of Partiels).
            //! @param resources The resources
            void makeUniqueIdentifiers(std::vector<Resource>& resources);

            //! @brief Gets the Markdown files (.md) of a directory and its subdirectories
            //! @param directory The directory
            //! @return The names (relative paths without extension) and the contents of the non-empty files sorted by path
            std::vector<std::pair<juce::String, juce::String>> getContextDocuments(juce::File const& directory);

            //! @brief The engine that indexes the resources and retrieves the relevant ones
            //! @details The resources are retrieved with the embedding model and an HNSW index, then sorted with the
            //! reranker model. The methods are thread-safe.
            class Engine
            {
            public:
                Engine() = default;
                ~Engine() = default;

                //! @brief Loads the embedding and reranker models
                //! @param embeddingModelFile The GGUF file of the embedding model
                //! @param rerankerModelFile The GGUF file of the reranker model
                //! @return The result of the initialization (the models are released on failure)
                juce::Result initializeModels(juce::File embeddingModelFile, juce::File rerankerModelFile);

                //! @brief Retrieves the resources relevant to a prompt
                //! @param userPrompt The prompt
                //! @param maxResources The maximum number of resources
                //! @param minScore The minimum reranker score of the resources
                //! @param maxDistance The maximum embedding distance of the candidates
                //! @return The resources sorted by relevance
                std::vector<Resource> computeResources(juce::String const& userPrompt, size_t maxResources = 4_z, float minScore = 0.01f, float maxDistance = 0.6f);

                //! @brief Gets the indexed resources matching identifiers
                //! @param ids The identifiers of the resources
                //! @return The resources in the order of the index
                std::vector<Resource> getResources(std::vector<juce::String> const& ids) const;

                //! @brief Computes the embeddings of the resources and builds the index
                //! @param resources The resources to index (they replace the previous ones)
                //! @return The result of the indexation
                juce::Result setResources(std::vector<Resource> const& resources);

                //! @brief Computes the normalized embedding of a text
                //! @param text The text
                //! @return The embedding or an empty vector on failure
                std::vector<float> computeEmbedding(juce::String const& text);

                //! @brief Computes the relevance of a resource for a query
                //! @param query The query
                //! @param resourceContent The content of the resource
                //! @return The relevance between 0 and 1
                float computeRerankerScore(juce::String const& query, juce::String const& resourceContent);

                //! @brief Searches the documentation (search_docs tool)
                //! @param query The query
                //! @param maxNumResources The maximum number of resources
                //! @param minScore The minimum reranker score of the resources
                //! @param maxDistance The maximum embedding distance of the candidates
                //! @return The MCP response listing the identifiers of the relevant resources (or an error response if
                //! the documentation cannot be searched)
                nlohmann::json searchDocs(std::string const& query, size_t maxNumResources, float minScore = 0.01f, float maxDistance = 0.6f);

                //! @brief Loads the content of resources (load_docs tool)
                //! @param ids The identifiers of the resources
                //! @return The MCP response with the content of the resources (or an error response if the
                //! documentation cannot be searched)
                nlohmann::json loadDocs(std::vector<std::string> const& ids);

                //! @brief Aborts the loading of the models and the indexation of the resources (thread-safe)
                //! @param state Whether the operations should be aborted (until the flag is reset)
                void setShouldQuit(bool state);

            private:
                //! @brief Checks whether the documentation can be searched (the models are loaded and the resources indexed)
                //! @return The result with the reason if the documentation cannot be searched
                juce::Result checkAvailability() const;
                //! @brief Computes the embedding of a text (the caller must lock mMutex)
                std::vector<float> computeEmbeddingUnlocked(juce::String const& text);
                //! @brief Computes the relevance score of a resource for a query (the caller must lock mMutex)
                float computeRerankerScoreUnlocked(juce::String const& query, juce::String const& resourceContent);

                std::atomic<bool> mShouldQuit{false};
                mutable std::mutex mMutex;
                std::vector<Resource> mIndexedEntries;
                std::unique_ptr<hnswlib::InnerProductSpace> mSpace;
                std::unique_ptr<hnswlib::HierarchicalNSW<float>> mIndex;
                llama_model_ptr mEmbeddingModel;
                llama_context_ptr mEmbeddingContext;
                llama_model_ptr mRerankerModel;
                llama_context_ptr mRerankerContext;
                bool mEmbeddingUsesEncoder = false; //!< Whether the embedding model is processed with the encoder (or the decoder)
                bool mRerankerUsesEncoder = false;  //!< Whether the reranker model is processed with the encoder (or the decoder)
            };
        } // namespace Rag
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
