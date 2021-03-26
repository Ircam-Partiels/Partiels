#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Zoom/AnlZoomPlayhead.h"
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
              sectionColourId = 0x2040100
        };
        
        GroupSection(Accessor& accessor, juce::Component& separator);
        ~GroupSection() override;
        
        // juce::Component
        void resized() override;
        
    private:
        class Container
        : public juce::Component
        {
        public:
            Container(Accessor& accessor, juce::Component& content, bool showPlayhead);
            ~Container() override;
            
            // juce::Component
            void resized() override;
            
        private:
            
            Accessor& mAccessor;
            juce::Component& mContent;
            Accessor::Listener mListener;
            
            Zoom::Playhead mZoomPlayhead;
        };

        Accessor& mAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;

        GroupThumbnail mThumbnail {mAccessor,};
        Decorator mThumbnailDecoration {mThumbnail, 1, 4.0f};
        
        GroupSnapshot mSnapshot {mAccessor,};
        Container mSnapshotContainer {mAccessor, mSnapshot, false};
        Decorator mSnapshotDecoration {mSnapshotContainer, 1, 4.0f};
        
        GroupPlot mPlot {mAccessor};
        Container mPlotContainer {mAccessor, mPlot, true};
        Decorator mPlotDecoration {mPlotContainer, 1, 4.0f};
        
        Zoom::Accessor zoomAcsr;
        Zoom::Ruler mRuler {zoomAcsr, Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mScrollBar {zoomAcsr, Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, true, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
