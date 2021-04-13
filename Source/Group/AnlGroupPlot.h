#pragma once

#include "AnlGroupTools.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "../Track/AnlTrackPlot.h"

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
        : public juce::Component
        , public Tooltip::BubbleClient
        {
        public:
            Overlay(Plot& plot);
            ~Overlay() override;
            
            // juce::Component
            void resized() override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            
        private:
            void updateTooltip(juce::Point<int> const& pt);
            
            Plot& mPlot;
            Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor::Listener mTimeZoomListener;
            Transport::PlayheadBar mTransportPlayheadBar;
        };
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        TrackMap<std::unique_ptr<Track::Plot>> mTrackPlots;
    };
}

ANALYSE_FILE_END
