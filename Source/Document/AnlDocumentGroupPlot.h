#pragma once

#include "AnlDocumentModel.h"
#include "../Transport/AnlTransportPlayheadContainer.h"
#include "../Track/AnlTrackPlot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupPlot
    : public juce::Component
    {
    public:
        GroupPlot(Accessor& accessor);
        ~GroupPlot() override;
        
        // juce::Component
        void resized() override;
        
        class Overlay
        : public juce::Component
        , public juce::SettableTooltipClient
        {
        public:
            Overlay(GroupPlot& groupPlot);
            ~Overlay() override = default;
            
            // juce::Component
            void resized() override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            
        private:
            GroupPlot& mGroupPlot;
            Accessor& mAccessor;
            Transport::PlayheadContainer mTransportPlayheadContainer;
            juce::Label mTooltip;
            juce::DropShadowEffect mDropShadowEffect;
        };
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        std::map<juce::String, std::unique_ptr<Track::Plot>> mPlots;
    };
}

ANALYSE_FILE_END
