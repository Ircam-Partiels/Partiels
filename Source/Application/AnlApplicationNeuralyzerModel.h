#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        static auto constexpr minContextSize = 4096;
        extern int const maxContextSize;
        static auto constexpr minBatchSize = 256;
        static auto constexpr maxBatchSize = 16384;
        static auto constexpr minTopK = 0;
        static auto constexpr maxTopK = 200;

        // clang-format off
        enum class AgentBackend
        {
              none
            , local
            , remote
        };
        // clang-format on

        struct ModelInfo
        {
            juce::File modelFile;
            juce::URL modelUrl{"http://localhost:1234"};
            juce::String modelId;
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
            explicit ModelInfo(juce::URL const& url);

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
              agentBackend
            , modelInfo
            , effectiveState
            , mcpForClaudeApp
            , mcpForCopilotApp
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::agentBackend, AgentBackend, Model::Flag::basic>
        , Model::Attr<AttrType::modelInfo, ModelInfo, Model::Flag::basic>
        , Model::Attr<AttrType::effectiveState, ModelInfo, Model::Flag::notifying>
        , Model::Attr<AttrType::mcpForClaudeApp, bool, Model::Flag::basic>
        , Model::Attr<AttrType::mcpForCopilotApp, bool, Model::Flag::basic>
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
                                          {AgentBackend::none}
                                        , {ModelInfo{}}
                                        , {ModelInfo{}}
                                        , {false}
                                        , {false}
                                    }))
            {
            }
            // clang-format on
        };

        void initializeEngine();
        void releaseEngine();

        juce::File resolveDirectory(juce::File const& root);
        juce::File getSessionFile(juce::File const& documentFile);
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
