#pragma once

#include "AnlTransportModel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    class PlayheadBar
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              startPlayheadColourId = 0x2020100
            , runningPlayheadColourId
        };
        
        PlayheadBar(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~PlayheadBar() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        double mStartPlayhead = 0.0;
        double mRunningPlayhead = 0.0;
    };
}

ANALYSE_FILE_END
