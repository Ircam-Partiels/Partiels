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
        InstantRenderer(Accessor& accessor);
        ~InstantRenderer() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
        void setTime(double time);
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
        double mTime = 0.0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstantRenderer)
    };
}

ANALYSE_FILE_END
