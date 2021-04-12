#pragma once

#include "AnlGroupTrackManager.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportModel.h"
#include "../Track/AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Snapshot
    : public juce::Component
    , private TrackManager<std::unique_ptr<Track::Snapshot>>
    {
    public:
        Snapshot(Accessor& groupAccessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Snapshot() override = default;
        
        // juce::Component
        void resized() override;
        
        class Overlay
        : public juce::Component
        , public Tooltip::BubbleClient
        {
        public:
            Overlay(Snapshot& snapshot);
            ~Overlay() override;
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            
        private:
            void updateTooltip(juce::Point<int> const& pt);
            
            Snapshot& mSnapshot;
            Accessor& mAccessor;
            Transport::Accessor& mTransportAccessor;
            Transport::Accessor::Listener mTransportListener;
        };
        
    private:
        // TrackManager<std::unique_ptr<Track::Snapshot>>
        void updateContentsStarted() override;
        void updateContentsEnded() override;
        void removeFromContents(std::unique_ptr<Track::Snapshot>& content) override;
        std::unique_ptr<Track::Snapshot> createForContents(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
    };
}

ANALYSE_FILE_END
