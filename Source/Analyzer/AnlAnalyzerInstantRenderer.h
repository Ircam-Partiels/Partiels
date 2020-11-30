#pragma once

#include "AnlAnalyzerPropertyPanel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class InstantRenderer
    : public juce::Component
    {
    public:
        InstantRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor);
        ~InstantRenderer() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
        void setTime(double time);
    private:
        Accessor& mAccessor;
        Zoom::Accessor& mZoomAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
        double mTime = 0.0;
        juce::Image mImage;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstantRenderer)
    };
}

ANALYSE_FILE_END
