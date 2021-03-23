#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Plot() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
        
    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Accessor::Listener mListener;
    };
}

ANALYSE_FILE_END
