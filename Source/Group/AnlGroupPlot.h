#pragma once

#include "../Track/AnlTrackPlot.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "../Zoom/AnlZoomModel.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Plot() override;

        // juce::Component
        void resized() override;

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
            bool mSnapshotMode{false};
        };

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        TrackMap<std::unique_ptr<Track::Plot>> mTrackPlots;
    };
} // namespace Group

ANALYSE_FILE_END
