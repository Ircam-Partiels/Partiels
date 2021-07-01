#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class Ruler
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2010000
            , gridColourId
            , anchorColourId
            , selectionColourId
        };
        
        enum Orientation : bool
        {
              vertical
            , horizontal
        };
        // clang-format on

        Ruler(Accessor& accessor, Orientation const orientation, std::function<juce::String(double)> valueToString = nullptr, int const maxStringWidth = 70);

        ~Ruler() override;

        std::function<void(void)> onDoubleClick = nullptr;
        std::function<bool(void)> onMouseDown = nullptr;

        // Juce::Component
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void paint(juce::Graphics& g) override;

    private:
        juce::Range<double> calculateSelectedValueRange(juce::MouseEvent const& event);

        // clang-format off
        enum class NavigationMode
        {
              zoom
            , translate
            , select
        };
        // clang-format on

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Grid::Accessor::Listener mGridListener{typeid(*this).name()};
        Orientation const mOrientation;
        std::function<juce::String(double)> mValueToString;
        int const mMaximumStringWidth = 60.0;

        juce::Range<double> mInitialValueRange;
        juce::Range<double> mSelectedValueRange{0.0, 0.0};
        juce::Point<float> mPrevMousePos{0.0, 0.0};

        NavigationMode mNavigationMode{NavigationMode::zoom};
    };
} // namespace Zoom

ANALYSE_FILE_END
