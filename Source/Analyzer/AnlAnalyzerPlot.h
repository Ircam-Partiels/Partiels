#pragma once

#include "AnlAnalyzerModel.h"
#include "AnlAnalyzerRenderer.h"
#include "../Zoom/AnlZoomPlayhead.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Plot
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x20005400
            , borderColourId
            , textColourId
        };
        
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Plot() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseEnter(juce::MouseEvent const& event) override;
        void mouseExit(juce::MouseEvent const& event) override;
        
    private:
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Renderer mRenderer;
        Accessor::Listener mListener;
        
        
        Zoom::Playhead mZoomPlayhead {mTimeZoomAccessor, {2, 2, 2, 2}};
        LoadingCircle mProcessingButton;
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
