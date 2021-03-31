#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Snapshot
    : public juce::Component
    {
    public:
        Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Snapshot() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public juce::Component
        , public juce::SettableTooltipClient
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
            Snapshot& mSnapshot;
            Accessor& mAccessor;
            Accessor::Listener mListener;
            
            juce::Label mTooltip;
        };
        
    private:
        static void paintMarker(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, double time);
        
        static void paintSegment(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, double time, juce::Range<double> const& valueRange);
        
        static void paintGrid(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<juce::Image> const& images, double time, Zoom::Accessor const& timeZoomAcsr, Zoom::Accessor const& binZoomAcsr);
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Accessor::Listener mListener;
    };
}

ANALYSE_FILE_END
