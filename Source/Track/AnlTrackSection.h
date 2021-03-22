#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlTrackThumbnail.h"
#include "AnlTrackPlot.h"
#include "AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Section
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
              sectionColourId = 0x2030100
        };
        
        Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator);
        ~Section() override;
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;
        
        Thumbnail mThumbnail {mAccessor};
        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        Plot mPlot {mAccessor, mTimeZoomAccessor};
        
        Zoom::Ruler mValueRuler {mAccessor.getAccessor<AcsrType::valueZoom>(0), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mValueScrollBar {mAccessor.getAccessor<AcsrType::valueZoom>(0), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::Ruler mBinRuler  {mAccessor.getAccessor<AcsrType::binZoom>(0), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mBinScrollBar {mAccessor.getAccessor<AcsrType::binZoom>(0), Zoom::ScrollBar::Orientation::vertical, true};
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, {50, 2000}};
    };
}

ANALYSE_FILE_END
