#pragma once

#include "AnlTrackPropertyPanel.h"
#include "AnlTrackRenderer.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Snapshot
    : public juce::Component
    {
    public:
        Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Snapshot() override;
        
        // juce::Component
        void paint(juce::Graphics& g) override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Renderer mRenderer;
        Accessor::Listener mListener;
    };
}

ANALYSE_FILE_END
