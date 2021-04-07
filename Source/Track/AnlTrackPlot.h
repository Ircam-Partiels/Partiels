#pragma once

#include "AnlTrackModel.h"
#include "../Transport/AnlTransportPlayheadContainer.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Plot() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        
        class Overlay
        : public juce::Component
        , public juce::SettableTooltipClient
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
            
        private:
            Plot& mPlot;
            Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Accessor::Listener mListener;
            Transport::PlayheadContainer mTransportPlayheadContainer;
            juce::Label mTooltip;
            juce::DropShadowEffect mDropShadowEffect;
        };
        
    private:
        static void paintMarkers(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange);
        
        static void paintSegments(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange);
        
        static void paintGrid(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<juce::Image> const& images, Zoom::Accessor const& timeZoomAcsr, Zoom::Accessor const& binZoomAcsr);
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Accessor::Listener mListener;
    };
}

ANALYSE_FILE_END
