#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    struct Grid
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

        static void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, juce::Justification justification);
        
        static void paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, double maxStringWidth, juce::Justification justification);
        
    private:
        using TickDrawingInfo = std::tuple<size_t, double, double, double, double>;
        static TickDrawingInfo getTickDrawingInfo(Accessor const& accessor, juce::Range<double> const& visibleRange, int size, double maxStringSize);
        static bool isMainTick(Accessor const& accessor, TickDrawingInfo const& info, double const value);
    };
} // namespace Zoom

ANALYSE_FILE_END
