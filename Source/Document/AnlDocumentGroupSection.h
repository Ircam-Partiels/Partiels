#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Transport/AnlTransportPlayheadContainer.h"
#include "AnlDocumentGroupThumbnail.h"
#include "AnlDocumentGroupSnapshot.h"
#include "AnlDocumentGroupPlot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupSection
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              sectionColourId = 0x2040000
        };
        
        GroupSection(Accessor& accessor);
        ~GroupSection() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;

        GroupThumbnail mThumbnail {mAccessor,};
        Decorator mThumbnailDecoration {mThumbnail, 1, 2.0f};
        
        GroupSnapshot mSnapshot {mAccessor,};
        GroupSnapshot::Overlay mSnapshotOverlay {mSnapshot};
        Decorator mSnapshotDecoration {mSnapshotOverlay, 1, 2.0f};
        
        GroupPlot mPlot {mAccessor};
        GroupPlot::Overlay mPlotOverlay {mPlot};
        Decorator mPlotDecoration {mPlotOverlay, 1, 2.0f};
        
        Zoom::Accessor zoomAcsr;
        Zoom::Ruler mRuler {zoomAcsr, Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mScrollBar {zoomAcsr, Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBar {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
