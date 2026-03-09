#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        // clang-format off
        enum class AttrType : size_t
        {
              modelFile
            , minP
            , temperature
            , contextSize
            , batchSize
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::modelFile, juce::File, Model::Flag::basic>
        , Model::Attr<AttrType::minP, float, Model::Flag::basic>
        , Model::Attr<AttrType::temperature, float, Model::Flag::basic>
        , Model::Attr<AttrType::contextSize, int32_t, Model::Flag::basic>
        , Model::Attr<AttrType::batchSize, int32_t, Model::Flag::basic>
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
                                          {juce::File{}}
                                        , {0.05f}
                                        , {0.2f}
                                        , {0}
                                        , {0}
                                    }))
            {
            }
            // clang-format on
        };

        struct ModelInfo
        {
            juce::File model;
            juce::File tplt;
            int32_t contextSize = 0;
            int32_t batchSize = 0;
            float minP = 0.05f;
            float temperature = 0.2f;

            ModelInfo() = default;
            ModelInfo(Accessor const& accessor);

            inline bool operator==(ModelInfo const& rhs) const noexcept
            {
                return model == rhs.model &&
                       tplt == rhs.tplt &&
                       contextSize == rhs.contextSize &&
                       batchSize == rhs.batchSize &&
                       std::abs(minP - rhs.minP) < std::numeric_limits<float>::epsilon() &&
                       std::abs(temperature - rhs.temperature) < std::numeric_limits<float>::epsilon();
            }

            inline bool operator!=(ModelInfo const& rhs) const noexcept
            {
                return !(*this == rhs);
            }

            inline bool valid() const
            {
                return model.existsAsFile() && contextSize > 0 && batchSize > 0;
            }
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
