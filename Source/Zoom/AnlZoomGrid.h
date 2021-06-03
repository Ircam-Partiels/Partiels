#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace Grid
    {
        // clang-format off
        enum class AttrType : size_t
        {
              tickReference
            , mainTickInterval
            , tickPowerBase
            , tickDivisionFactor
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::tickReference, double, Model::Flag::basic>
        , Model::Attr<AttrType::mainTickInterval, size_t, Model::Flag::basic>
        , Model::Attr<AttrType::tickPowerBase, double, Model::Flag::basic>
        , Model::Attr<AttrType::tickDivisionFactor, double, Model::Flag::basic>
        >;
        // clang-format on

        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;

            Accessor(double tickReference = 0.0, size_t mainTickInterval = 3_z, double tickPowerBase = 2.0, double tickDivisionFactor = 5.0)
            : Accessor(AttrContainer({tickReference}, {mainTickInterval}, {tickPowerBase}, {tickDivisionFactor}))
            {
            }
        };

        void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, juce::Justification justification);
    } // namespace Grid
} // namespace Zoom

ANALYSE_FILE_END
