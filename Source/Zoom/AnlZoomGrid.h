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

        using Justification = juce::Justification;

        static void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, Justification justification);

        static void paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, int maxStringWidth, Justification justification);

        class PropertyPanel
        : public FloatingWindowContainer
        , public juce::DragAndDropContainer
        {
        public:
            PropertyPanel();
            ~PropertyPanel() override;

            // juce::Component
            void resized() override;

            void setGrid(Accessor const& accessor);
            std::function<void(Accessor const& accessor)> onChanged = nullptr;
            std::function<void()> onShow = nullptr;
            std::function<void()> onHide = nullptr;

        private:
            // FloatingWindowContainer
            void hide() override;
            void showAt(juce::Point<int> const& pt) override;

            Accessor mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};

            PropertyNumber mPropertyTickReference;
            PropertyList mPropertyTickBase;
            PropertyNumber mPropertyMainTickInterval;
            PropertyNumber mPropertyTickPowerBase;
            PropertyNumber mPropertyTickDivisionFactor;

            static auto constexpr sInnerWidth = 300;
            using GridBaseInfo = std::tuple<size_t, double, double>;
            // clang-format off
            static constexpr std::array<GridBaseInfo, 6> sGridBaseInfoArray
            {
                  GridBaseInfo{0_z, 10.0, 4.0}
                , GridBaseInfo{0_z, 2.0, 5.0}
                , GridBaseInfo{3_z, 2.0, 5.0}
                , GridBaseInfo{0_z, 4.0, 4.0}
                , GridBaseInfo{0_z, 10.0, 2.0}
                , GridBaseInfo{0_z, 6.0, 5.0}
            };
            // clang-format on
        };

    private:
        using TickDrawingInfo = std::tuple<size_t, double, double, double, double>;
        static TickDrawingInfo getTickDrawingInfo(Accessor const& accessor, juce::Range<double> const& visibleRange, int size, double maxStringSize);
        static bool isMainTick(Accessor const& accessor, TickDrawingInfo const& info, double const value);
    };
} // namespace Zoom

ANALYSE_FILE_END
