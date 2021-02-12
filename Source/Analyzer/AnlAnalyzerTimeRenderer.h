#pragma once

#include "AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomPlayhead.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class TimeRenderer
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x20005400
            , borderColourId
            , textColourId
        };
        
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
        
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        Zoom::Playhead mZoomPlayhead {mTimeZoomAccessor, {2, 2, 2, 2}};
        
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
