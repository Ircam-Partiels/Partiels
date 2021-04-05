#pragma once

#include "AnlTransportModel.h"
#include "../Zoom/AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    class PlayheadContainer
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              startPlayheadColourId = 0x2020000
            , runningPlayheadColourId
        };
        
        PlayheadContainer(Accessor& accessor, Zoom::Accessor& zoomAcsr);
        ~PlayheadContainer() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        
    private:
        int toPixel(double time) const;
        double toTime(int pixel) const;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Zoom::Accessor& mZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        double mStartPlayhead = 0.0;
        double mRunningPlayhead = 0.0;
    };
}

ANALYSE_FILE_END
