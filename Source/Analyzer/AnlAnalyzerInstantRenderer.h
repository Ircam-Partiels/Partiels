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

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mZoomAccessor;
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;
        
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
