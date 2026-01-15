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
            , contextSize
            , batchSize
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::modelFile, juce::File, Model::Flag::basic>
        , Model::Attr<AttrType::contextSize, uint32_t, Model::Flag::basic>
        , Model::Attr<AttrType::batchSize, uint32_t, Model::Flag::basic>
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
                                        , {0u}
                                        , {0u}
                                    }))
            {
            }
            // clang-format on
        };

        struct ModelInfo
        {
            juce::File model;
            juce::File tplt;
            uint32_t contextSize = 0u;
            uint32_t batchSize = 0u;

            ModelInfo() = default;
            ModelInfo(Accessor const& accessor);

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

            inline bool valid() const
            {
                return model.existsAsFile() && contextSize > 0u && batchSize > 0u;
            }
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
