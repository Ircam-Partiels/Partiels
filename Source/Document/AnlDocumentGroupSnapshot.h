#pragma once

#include "AnlDocumentModel.h"
#include "../Track/AnlTrackSnapshot.h"


ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupSnapshot
    : public juce::Component
    {
    public:
        GroupSnapshot(Accessor& accessor);
        ~GroupSnapshot() override;
        
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
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        std::map<juce::String, std::unique_ptr<Track::Snapshot>> mSnapshots;
    };
}

ANALYSE_FILE_END
