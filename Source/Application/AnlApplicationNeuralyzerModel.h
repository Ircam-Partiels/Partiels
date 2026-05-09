#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        static auto constexpr minContextSize = 4096;
        static auto constexpr maxContextSize = std::numeric_limits<int32_t>::max();
        static auto constexpr minBatchSize = 256;
        static auto constexpr maxBatchSize = 16384;
        static auto constexpr minTopK = 0;
        static auto constexpr maxTopK = 200;

        // clang-format off
        enum class ModelBackend
        {
              local
            , remote
        };
        // clang-format on

        struct ModelInfo
        {
            juce::File modelFile;
            juce::String modelId;
            juce::URL modelUrl{"http://localhost:1234"};
            std::optional<int32_t> contextSize;
            std::optional<int32_t> batchSize;
            std::optional<float> minP;
            std::optional<float> temperature;
            std::optional<float> topP;
            std::optional<int32_t> topK;
            std::optional<float> presencePenalty;
            std::optional<float> repetitionPenalty;

            ModelInfo() = default;
            explicit ModelInfo(juce::File const& file);
            explicit ModelInfo(juce::String const& model);

            bool operator==(ModelInfo const& rhs) const noexcept;

            inline bool operator!=(ModelInfo const& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            inline bool isValid() const
            {
                return modelFile.existsAsFile() || (modelUrl.isWellFormed() && modelId.isNotEmpty());
            }
        };

        // clang-format off
        enum class AttrType : size_t
        {
              modelInfo
            , modelBackend
            , effectiveState
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::modelInfo, ModelInfo, Model::Flag::basic>
        , Model::Attr<AttrType::modelBackend, ModelBackend, Model::Flag::notifying>
        , Model::Attr<AttrType::effectiveState, ModelInfo, Model::Flag::notifying>
        >;
        // clang-format on

        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;
            // clang-format off
            Accessor()
            : Accessor(AttrContainer({
                                          {ModelInfo{}}
                                        , {ModelBackend::local}
                                        , {ModelInfo{}}
                                    }))
            {
            }
            // clang-format on
        };

        juce::File resolveNeuralyzerDirectory(juce::File const& root);
        juce::File getDefaultModelDirectory();
        juce::File getRagEmbeddingModelFile();
        juce::File getRagRerankerModelFile();
        juce::File getNeuralyzerSessionFile(juce::File const& documentFile);
    } // namespace Neuralyzer
} // namespace Application

namespace XmlParser
{
    template <>
    void toXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& value);

    template <>
    auto fromXml<Application::Neuralyzer::ModelInfo>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Application::Neuralyzer::ModelInfo const& defaultValue)
        -> Application::Neuralyzer::ModelInfo;
} // namespace XmlParser

ANALYSE_FILE_END
