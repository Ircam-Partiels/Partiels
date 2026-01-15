#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        struct ModelInfo
        {
            juce::File model;
            juce::File tplt;
            uint32_t contextSize = 0;
            uint32_t batchSize = 0;

            inline bool operator==(ModelInfo const& rhs) const noexcept
            {
                return model == rhs.model &&
                       tplt == rhs.tplt &&
                       contextSize == rhs.contextSize &&
                       batchSize == rhs.batchSize;
            }

            inline bool operator!=(ModelInfo const& rhs) const noexcept
            {
                return !(*this == rhs);
            }
        };

        void to_json(nlohmann::json& j, ModelInfo const& modelInfo);
        void from_json(nlohmann::json const& j, ModelInfo& modelInfo);

        // clang-format off
        enum class AttrType : size_t
        {
              modelInfo
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::modelInfo, ModelInfo, Model::Flag::basic>
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
                                    }))
            {
            }
            // clang-format on
        };
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
