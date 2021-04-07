#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlTrackThumbnail.h"
#include "AnlTrackPlot.h"
#include "AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Section
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              sectionColourId = 0x2030000
        };
        
        Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr, juce::Component& separator);
        ~Section() override;
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;
        
        Thumbnail mThumbnail {mAccessor};
        Decorator mThumbnailDecoration {mThumbnail, 1, 4.0f};
        
        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        Snapshot::Overlay mSnapshotOverlay {mSnapshot};
        Decorator mSnapshotDecoration {mSnapshotOverlay, 1, 4.0f};
        
        Plot mPlot {mAccessor, mTimeZoomAccessor, mTransportAccessor};
        Plot::Overlay mPlotOverlay {mPlot};
        Decorator mPlotDecoration {mPlotOverlay, 1, 4.0f};
        
        Zoom::Ruler mValueRuler {mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mValueScrollBar {mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::Ruler mBinRuler  {mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mBinScrollBar {mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, true, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
