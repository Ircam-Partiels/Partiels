#pragma once

#include "AnlAnalyzerPropertyPanel.h"
#include "../Zoom/AnlZoomModel.h"

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
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;
        
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mZoomListener;
        juce::Label mInformation;
    };
}

ANALYSE_FILE_END
