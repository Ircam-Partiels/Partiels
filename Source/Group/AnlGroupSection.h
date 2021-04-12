#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "AnlGroupThumbnail.h"
#include "AnlGroupSnapshot.h"
#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Section
    : public juce::Component
    {
    public:
        Section(Accessor& accessor, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Section() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener;

        Thumbnail mThumbnail {mAccessor};
        Decorator mThumbnailDecoration {mThumbnail, 1, 2.0f};
        
        Snapshot mSnapshot {mAccessor, mTransportAccessor, mTimeZoomAccessor};
        Snapshot::Overlay mSnapshotOverlay {mSnapshot};
        Decorator mSnapshotDecoration {mSnapshotOverlay, 1, 2.0f};
        
        Plot mPlot {mAccessor, mTransportAccessor, mTimeZoomAccessor};
        Plot::Overlay mPlotOverlay {mPlot};
        Decorator mPlotDecoration {mPlotOverlay, 1, 2.0f};
        
        Zoom::Accessor zoomAcsr;
        Zoom::Ruler mRuler {zoomAcsr, Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mScrollBar {zoomAcsr, Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBar {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
