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

        private:
            void updateTooltip(juce::Point<int> const& pt);
            void updateMode(juce::MouseEvent const& event);

            Plot& mPlot;
            Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
            SelectionBar mSelectionBar;
            bool mSnapshotMode{false};
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
