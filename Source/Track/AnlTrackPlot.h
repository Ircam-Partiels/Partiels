#pragma once

#include "../Transport/AnlTransportPlayheadBar.h"
#include "AnlTrackModel.h"
#include "AnlTrackGrid.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Plot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public ComponentSnapshot
        , public Tooltip::BubbleClient
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
            Accessor::Listener mListener;
            Zoom::Accessor::Listener mTimeZoomListener;
            Transport::PlayheadBar mTransportPlayheadBar;
            Grid mGrid{mAccessor};
            bool mSnapshotMode{false};
        };

        static void paint(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> bounds, Zoom::Accessor const& timeZoomAcsr);

    private:
        static void paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        static void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        static void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor::Listener mZoomListener;
        Accessor::Listener mListener;
    };
} // namespace Track

ANALYSE_FILE_END
