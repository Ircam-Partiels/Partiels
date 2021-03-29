#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Plot() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        
    private:
        static void paintMarkers(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange);
        
        static void paintSegments(juce::Graphics& g, juce::Rectangle<float> const& bounds, juce::Colour const& colour, std::vector<Plugin::Result> const& results, juce::Range<double> const& timeRange, juce::Range<double> const& valueRange);
        
        static void paintGrid(juce::Graphics& g, juce::Rectangle<int> const& bounds, std::vector<juce::Image> const& images, Zoom::Accessor const& timeZoomAcsr, Zoom::Accessor const& binZoomAcsr);
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Accessor::Listener mListener;
    };
}

ANALYSE_FILE_END
