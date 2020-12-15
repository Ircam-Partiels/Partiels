#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlAnalyzerThumbnail.h"
#include "AnlAnalyzerTimeRenderer.h"
#include "AnlAnalyzerInstantRenderer.h"
#include "AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Section
    : public juce::Component
    , private juce::ComponentListener
    {
    public:
        
        enum ColourIds : int
        {
              sectionColourId = 0x2000340
        };
        
        Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator);
        ~Section() override;
        
        std::function<void(void)> onRemove = nullptr;
        
        void setTime(double time);
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        // juce::ComponentListener
        void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
        
        Analyzer::Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        juce::Component& mSeparator;
        Analyzer::Accessor::Listener mListener;
        
        Analyzer::Thumbnail mThumbnail {mAccessor};
        Analyzer::InstantRenderer mInstantRenderer {mAccessor, mTimeZoomAccessor};
        Analyzer::TimeRenderer mTimeRenderer {mAccessor, mTimeZoomAccessor};
        
        std::unique_ptr<Zoom::Ruler> mRuler;
        std::unique_ptr<Zoom::ScrollBar> mScrollbar;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

ANALYSE_FILE_END
