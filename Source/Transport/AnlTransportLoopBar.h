#pragma once

#include "../Zoom/AnlZoomModel.h"
#include "AnlTransportModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    class LoopBar
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2020000
            , thumbCoulourId
        };
        // clang-format on

        LoopBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~LoopBar() override;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;

    private:
        // clang-format off
        enum class EditMode
        {
              none
            , select
            , drag
            , resizeLeft
            , resizeRight
        };
        // clang-format on

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        juce::Range<double> mLoopRange;
        double mCickTime = 0.0;
        juce::Range<double> mSavedRange;
        EditMode mEditMode = EditMode::none;
    };
} // namespace Transport

ANALYSE_FILE_END
