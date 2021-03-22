#pragma once

#include "AnlDocumentModel.h"
#include "../Zoom/AnlZoomPlayhead.h"
#include "../Track/AnlTrackRenderer.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Plot
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x2050000
            , borderColourId
            , textColourId
        };
        
        Plot(Accessor& accessor);
        ~Plot() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Track::Accessor::Listener mAnalyzerListener;
        Zoom::Accessor::Listener mZoomListener;
        Zoom::Accessor::Listener mTimeZoomListener;
        Zoom::Playhead mZoomPlayhead;
        LoadingCircle mProcessingButton;
        
        ResizerBar mResizerBar {ResizerBar::Orientation::horizontal, {50, 2000}};
        
        using AnlAcsrRef = std::reference_wrapper<Track::Accessor>;
        std::vector<std::tuple<AnlAcsrRef, std::unique_ptr<Track::Renderer>>> mRenderers;
    };
}

ANALYSE_FILE_END
