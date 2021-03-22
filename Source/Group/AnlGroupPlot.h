#pragma once

#include "AnlGroupModel.h"
#include "../Zoom/AnlZoomPlayhead.h"
#include "../Track/AnlTrackRenderer.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Plot
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              backgroundColourId = 0x2040000
            , borderColourId
        };
        
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Plot() override;
        
        void addAnalyzer(Analyzer::Accessor& anlAcsr);
        void removeAnalyzer(Analyzer::Accessor& anlAcsr);
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener;
        
        Analyzer::Accessor::Listener mAnalyzerListener;
        Zoom::Accessor::Listener mZoomListener;
        Zoom::Playhead mZoomPlayhead {mTimeZoomAccessor, {2, 2, 2, 2}};
        LoadingCircle mProcessingButton;
        
        using AnlAcsrRef = std::reference_wrapper<Analyzer::Accessor>;
        struct AnlAcsrRefComp
        {
            bool operator()(AnlAcsrRef const& lhs, AnlAcsrRef const& rhs) const
            {
                return &(lhs.get()) < &(rhs.get());
            }
        };
        std::map<AnlAcsrRef, std::unique_ptr<Analyzer::Renderer>, AnlAcsrRefComp> mRenderers;
    };
}

ANALYSE_FILE_END
