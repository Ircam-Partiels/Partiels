#pragma once

#include "AnlTrackDirector.h"
#include "AnlTrackRuler.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Plot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public ComponentSnapshot
        , public Tooltip::BubbleClient
        , public juce::SettableTooltipClient
        {
        public:
            Overlay(Plot& plot);
            ~Overlay() override;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void mouseDrag(juce::MouseEvent const& event) override;
            void mouseUp(juce::MouseEvent const& event) override;
            void mouseDoubleClick(juce::MouseEvent const& event) override;
            void modifierKeysChanged(juce::ModifierKeys const& modifiers) override;

            // ComponentSnapshot
            void takeSnapshot() override;

        private:
            // clang-format off
            enum class ActionMode
            {
                  none
                , create
                , move
            };
            // clang-format on

            void updateActionMode(juce::Point<int> const& point, juce::ModifierKeys const& modifiers);
            void updateTooltip(juce::Point<int> const& pt);

            Plot& mPlot;
            Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
            SelectionBar mSelectionBar;
            ActionMode mActionMode;
            bool mMouseWasDragged{false};
            double mMouseDownTime{0.0};
            Edition mCurrentEdition;
        };

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
    };
} // namespace Track

ANALYSE_FILE_END
