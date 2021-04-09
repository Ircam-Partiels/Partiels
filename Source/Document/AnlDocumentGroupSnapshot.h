#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupContainer.h"
#include "../Track/AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupSnapshot
    : public juce::Component
    , private GroupContainer<std::unique_ptr<Track::Snapshot>>
    {
    public:
        GroupSnapshot(Accessor& accessor);
        ~GroupSnapshot() override = default;
        
        // juce::Component
        void resized() override;
        
        class Overlay
        : public juce::Component
        , public Tooltip::BubbleClient
        {
        public:
            Overlay(GroupSnapshot& groupSnapshot);
            ~Overlay() override;
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            
        private:
            void updateTooltip(juce::Point<int> const& pt);
            
            GroupSnapshot& mGroupSnapshot;
            Accessor& mAccessor;
            Transport::Accessor::Listener mTransportListener;
        };
        
    private:
        // GroupContainer<std::unique_ptr<Track::Snapshot>>
        void updateStarted() override;
        void updateEnded() override;
        void removeFromGroup(std::unique_ptr<Track::Snapshot>& value) override;
        std::unique_ptr<Track::Snapshot> createForGroup(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
    };
}

ANALYSE_FILE_END
