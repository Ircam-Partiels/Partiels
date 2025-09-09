#pragma once

#include "../MiscModel.h"
#include "../MiscPropertyComponent.h"

MISC_FILE_BEGIN

namespace Zoom
{
    struct Grid
    {
        class Justification
        {
        public:
            Justification(int justificationFlags) noexcept
            : mFlags(justificationFlags)
            {
            }

            Justification(const Justification&) = default;
            Justification& operator=(const Justification&) = default;
            bool operator==(Justification const& other) const noexcept { return mFlags == other.mFlags; }
            bool operator!=(Justification const& other) const noexcept { return mFlags != other.mFlags; }
            inline int getFlags() const noexcept { return mFlags; }
            inline bool testFlags(int flagsToTest) const noexcept { return (mFlags & flagsToTest) != 0; }
            int getOnlyVerticalFlags() const noexcept { return mFlags & (top | bottom | vertical); }
            int getOnlyHorizontalFlags() const noexcept { return mFlags & (left | right | horizontal); }
            juce::Justification getTextJustification() { return juce::Justification(mFlags); }

            // clang-format off
            enum Flags
            {
                  none = 0
                , left = 1
                , right = 2
                , horizontal = 4
                , top = 8
                , bottom = 16
                , vertical = 32
            };
            // clang-format on

        private:
            int mFlags;
        };

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

        static void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, Justification justification);

        static void paintHorizontal(juce::Graphics& g, Accessor const& accessor, juce::Range<double> const& visibleRange, juce::Rectangle<int> const& bounds, std::function<juce::String(double)> const stringify, int maxStringWidth, Justification justification);

        class PropertyPanel
        : public juce::Component
        , public juce::DragAndDropContainer
        {
        public:
            PropertyPanel();
            ~PropertyPanel() override;

            // juce::Component
            void resized() override;

            void setGrid(Accessor const& accessor);
            std::function<void(Accessor const& accessor)> onChangeBegin = nullptr;
            std::function<void(Accessor const& accessor)> onChangeEnd = nullptr;

        private:
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

MISC_FILE_END
