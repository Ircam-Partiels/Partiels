#pragma once

#include "AnlAnalyzerPropertyPanel.h"
#include "AnlAnalyzerRenderer.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Snapshot
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x20005300
            , borderColourId
            , textColourId
        };
        
        Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Snapshot() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Renderer mRenderer;
        Accessor::Listener mListener;
        
        
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
