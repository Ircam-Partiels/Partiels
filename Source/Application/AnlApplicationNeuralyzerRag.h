#pragma once

#include "AnlApplicationNeuralyzerAst.h"
#include "AnlApplicationNeuralyzerMcp.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wunused-function", "-Wextra-semi", "-Wshadow", "-Wsign-conversion", "-Wshorten-64-to-32", "-Wimplicit-int-float-conversion", "-Wimplicit-int-conversion", "-Wconditional-uninitialized", "-Wcast-align")
#include <hnswlib/hnswlib.h>
#include <llama-cpp.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Rag
        {
            juce::File getDefaultModelDirectory();
            juce::URL getEmbeddingModelUrl();
            juce::URL getRerankerModelUrl();
            juce::File getEmbeddingModelFile();
            juce::File getRerankerModelFile();

            void downloadModelsIfNecessary();

            struct Resource
            {
                juce::String id;
                juce::String content;
                juce::String section;
                juce::String document;
                float score = 0.0f;

                bool operator==(Resource const& rhs) const noexcept;
                bool operator<(Resource const& rhs) const;

                friend void to_json(nlohmann::json& json, Resource const& resource);
                friend void from_json(nlohmann::json const& json, Resource& resource);
            };

            std::vector<Resource> createResources(Ast::Document const& document, int maxResourceLength);

            class Engine
            {
            public:
                Engine(Mcp::Dispatcher& mcpDispatcher);
                ~Engine() = default;

                juce::Result initializeModels(juce::File embeddingModelFile, juce::File rerankerModelFile);
                std::vector<Resource> computeResources(juce::String const& userPrompt, size_t maxResources = 4_z, float minScore = 0.01f, float maxDistance = 0.6f);
                std::vector<Resource> getResources(std::vector<juce::String> const& ids) const;

            private:
                juce::Result setResources(std::vector<Resource> const& resources);
                std::vector<float> computeEmbedding(juce::String const& text);
                float computeRerankerScore(juce::String const& query, juce::String const& resourceContent);

                Mcp::Dispatcher& mMcpDispatcher;
                std::mutex mMutex;
                std::vector<Resource> mIndexedEntries;
                std::unique_ptr<hnswlib::InnerProductSpace> mSpace;
                std::unique_ptr<hnswlib::HierarchicalNSW<float>> mIndex;
                llama_model_ptr mEmbeddingModel;
                llama_context_ptr mEmbeddingContext;
                llama_model_ptr mRerankerModel;
                llama_context_ptr mRerankerContext;
            };
        } // namespace Rag
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
