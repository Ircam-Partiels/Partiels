#pragma once

#include "MiscTransportModel.h"

MISC_FILE_BEGIN

namespace Transport
{
    class PlayheadBar
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              startPlayheadColourId = 0x2020000
            , runningPlayheadColourId
        };
        // clang-format on

        PlayheadBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~PlayheadBar() override;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

        std::function<void(double)> onStartPlayheadChanged = nullptr;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        double mStartPlayhead = 0.0;
        double mRunningPlayhead = 0.0;
    };

    class SelectionBar
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2020100
            , thumbCoulourId
        };
        // clang-format on

        SelectionBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~SelectionBar() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

        void setDefaultMouseCursor(juce::MouseCursor const& cursor);

        std::function<void(juce::Range<double> const&)> onSelectionRangeChanged = nullptr;

    private:
        using Anchor = Zoom::SelectionBar::Anchor;

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::SelectionBar mSelectionBar;
        PlayheadBar mPlayheadBar;
    };

    class LoopBar
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2020200
            , thumbCoulourId
        };
        // clang-format on

        LoopBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~LoopBar() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

        std::function<void(juce::Range<double> const&)> onLoopRangeChanged = nullptr;

    private:
        using Anchor = Zoom::SelectionBar::Anchor;

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor& mZoomAccessor;
        Zoom::SelectionBar mSelectionBar;
        PlayheadBar mPlayheadBar;
    };
} // namespace Transport

MISC_FILE_END
