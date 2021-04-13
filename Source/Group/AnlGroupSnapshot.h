#pragma once

#include "AnlGroupTools.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Transport/AnlTransportModel.h"
#include "../Track/AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Snapshot
    : public juce::Component
    {
    public:
        Snapshot(Accessor& groupAccessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Snapshot() override;
        
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
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        TrackMap<std::unique_ptr<Track::Snapshot>> mTrackSnapshots;
    };
}

ANALYSE_FILE_END
