#pragma once

#include "../Misc/AnlMisc.h"
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
            struct Resource
            {
                juce::String id;
                juce::String content;
                juce::String section;
                juce::String document;

                bool operator==(Resource const& rhs) const noexcept;
                bool operator<(Resource const& rhs) const;

                friend void to_json(nlohmann::json& json, Resource const& resource);
                friend void from_json(nlohmann::json const& json, Resource& resource);
            };

            struct Context
            {
                juce::String excerpt;
                juce::String section;
                juce::String document;
            };

            class Engine
            {
            public:
                Engine() = default;
                ~Engine() = default;

                juce::Result initializeModel(juce::File modelFile);
                juce::Result setResources(std::set<Resource> resources);
                std::vector<Context> computeContext(juce::String const& userPrompt, float maxDistance = std::numeric_limits<float>::max());

            private:
                std::vector<float> computeEmbedding(juce::String const& text);

                juce::File mModelFile;
                std::mutex mMutex;
                std::set<Resource> mResourceEntries;
                std::vector<Resource> mIndexedEntries;
                std::unique_ptr<hnswlib::InnerProductSpace> mSpace;
                std::unique_ptr<hnswlib::HierarchicalNSW<float>> mIndex;
                llama_model_ptr mEmbeddingModel;
                llama_context_ptr mEmbeddingContext;
            };
        } // namespace Rag
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
