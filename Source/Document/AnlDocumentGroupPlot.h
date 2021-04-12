#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupContainer.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "../Track/AnlTrackPlot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupPlot
    : public juce::Component
    , private GroupContainer<std::unique_ptr<Track::Plot>>
    {
    public:
        GroupPlot(Accessor& accessor);
        ~GroupPlot() override  = default;
        
        // juce::Component
        void resized() override;
        
        class Overlay
        : public juce::Component
        , public Tooltip::BubbleClient
        {
        public:
            Overlay(GroupPlot& groupPlot);
            ~Overlay() override;
            
            // juce::Component
            void resized() override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            
        private:
            void updateTooltip(juce::Point<int> const& pt);
            
            GroupPlot& mGroupPlot;
            Accessor& mAccessor;
            Zoom::Accessor::Listener mTimeZoomListener;
            Transport::PlayheadBar mTransportPlayheadBar;
        };
        
    private:
        // GroupContainer<std::unique_ptr<Track::Plot>>
        void updateStarted() override;
        void updateEnded() override;
        void removeFromGroup(std::unique_ptr<Track::Plot>& value) override;
        std::unique_ptr<Track::Plot> createForGroup(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
    };
}

ANALYSE_FILE_END
