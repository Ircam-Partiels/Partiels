#pragma once

#include "AnlAnalyzerPropertyPanel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class TimeRenderer
    : public juce::Component
    {
    public:        
        TimeRenderer(Accessor& accessor, Zoom::Accessor& zoomAccessor);
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
        
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
