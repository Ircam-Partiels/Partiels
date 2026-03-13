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
            std::optional<int32_t> contextSize;
            std::optional<int32_t> batchSize;
            std::optional<float> minP;
            std::optional<float> temperature;

            ModelInfo() = default;

            inline bool operator==(ModelInfo const& rhs) const noexcept
            {
                static auto const equals = [](std::optional<float> const& lhsf, std::optional<float> const& rhsf)
                {
                    return lhsf.has_value() == rhsf.has_value() && (!lhsf.has_value() || std::abs(lhsf.value() - rhsf.value()) < std::numeric_limits<float>::epsilon());
                };

                return model == rhs.model &&
                       tplt == rhs.tplt &&
                       contextSize == rhs.contextSize &&
                       batchSize == rhs.batchSize &&
                       equals(minP, rhs.minP) &&
                       equals(temperature, rhs.temperature);
            }

            inline bool operator!=(ModelInfo const& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            inline bool valid() const
            {
                return model.existsAsFile();
            }
        };

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
