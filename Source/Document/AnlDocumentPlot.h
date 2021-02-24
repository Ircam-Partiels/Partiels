#pragma once

#include "AnlDocumentModel.h"
#include "../Zoom/AnlZoomPlayhead.h"
#include "../Analyzer/AnlAnalyzerRenderer.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Plot
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x20005400
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
        
        Analyzer::Accessor::Listener mAnalyzerListener;
        Zoom::Accessor::Listener mZoomListener;
        Zoom::Playhead mZoomPlayhead;
        LoadingCircle mProcessingButton;
        
        using AnlAcsrRef = std::reference_wrapper<Analyzer::Accessor>;
        std::vector<std::tuple<AnlAcsrRef, std::unique_ptr<Analyzer::Renderer>>> mRenderers;
    };
}

ANALYSE_FILE_END
