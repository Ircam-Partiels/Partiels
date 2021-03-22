#pragma once

#include "AnlDocumentModel.h"
#include "../Zoom/AnlZoomPlayhead.h"
#include "../Track/AnlTrackPlot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Plot
    : public juce::Component
    {
    public:
        
        Plot(Accessor& accessor);
        ~Plot() override;
        
        // juce::Component
        void resized() override;
        
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
    
        Zoom::Playhead mZoomPlayhead;
        LoadingCircle mProcessingButton;
        
        std::map<juce::String, std::unique_ptr<Track::Plot>> mPlots;
    };
}

ANALYSE_FILE_END
