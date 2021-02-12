#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlAnalyzerThumbnail.h"
#include "AnlAnalyzerTimeRenderer.h"
#include "AnlAnalyzerSnapshot.h"

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
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        // juce::ComponentListener
        void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
        
        Accessor& mAccessor;
        Plot::Accessor& mPlotAccessor {mAccessor.getAccessor<AcsrType::plot>(0)};
        Zoom::Accessor& mTimeZoomAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        Plot::Accessor::Listener mPlotListener;
        
        Thumbnail mThumbnail {mAccessor};
        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        TimeRenderer mTimeRenderer {mAccessor, mTimeZoomAccessor};
        
        Zoom::Ruler mValueRuler {mPlotAccessor.getAccessor<Plot::AcsrType::valueZoom>(0), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mValueScrollBar {mPlotAccessor.getAccessor<Plot::AcsrType::valueZoom>(0), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::Ruler mBinRuler  {mPlotAccessor.getAccessor<Plot::AcsrType::binZoom>(0), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mBinScrollBar {mPlotAccessor.getAccessor<Plot::AcsrType::binZoom>(0), Zoom::ScrollBar::Orientation::vertical, true};
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, {50, 2000}};
    };
}

ANALYSE_FILE_END
