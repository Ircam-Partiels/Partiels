#pragma once

#include "AnlGroupTrackManager.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "../Track/AnlTrackPlot.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Plot
    : public juce::Component
    , private TrackManager<std::unique_ptr<Track::Plot>>
    {
    public:
        Plot(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Plot() override  = default;
        
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
        // TrackManager<std::unique_ptr<Track::Plot>>
        void updateContentsStarted() override;
        void updateContentsEnded() override;
        void removeFromContents(std::unique_ptr<Track::Plot>& content) override;
        std::unique_ptr<Track::Plot> createForContents(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
    };
}

ANALYSE_FILE_END
