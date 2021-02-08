#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class TimeRenderer
    : public juce::Component
    {
    public:        
        TimeRenderer(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~TimeRenderer() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseEnter(juce::MouseEvent const& event) override;
        void mouseExit(juce::MouseEvent const& event) override;
        
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;
        Plot::Accessor::Listener mPlotListener;
        
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
