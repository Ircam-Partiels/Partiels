#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Zoom/AnlZoomPlayhead.h"
//#include "AnlTrackThumbnail.h"
#include "AnlDocumentGroupPlot.h"
//#include "AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupSection
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              sectionColourId = 0x2030100
        };
        
        GroupSection(Accessor& accessor, size_t index, juce::Component& separator);
        ~GroupSection() override;
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        class Container
        : public juce::Component
        {
        public:
            Container(Accessor& accessor, size_t index, juce::Component& content, bool showPlayhead);
            ~Container() override;
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            
        private:
            
            Accessor& mAccessor;
            size_t const mIndex;
            juce::Component& mContent;
            Accessor::Listener mListener;
            
            Zoom::Playhead mZoomPlayhead;
        };

        Accessor& mAccessor;
        size_t const mIndex;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;

        juce::Component mThumbnail;
        
        juce::Component mSnapshot;
        Container mSnapshotContainer {mAccessor, mIndex, mSnapshot, false};
        Decorator mSnapshotDecoration {mSnapshotContainer, 1, 4.0f};
        
        
//        Thumbnail mThumbnail {mAccessor};
//
//        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
//        Container mSnapshotContainer {mAccessor, mTimeZoomAccessor, mSnapshot, false};
//        Decorator mSnapshotDecoration {mSnapshotContainer, 1, 4.0f};
//
        GroupPlot mPlot {mAccessor};
        Container mPlotContainer {mAccessor, mIndex, mPlot, true};
        Decorator mPlotDecoration {mPlotContainer, 1, 4.0f};
        
        Zoom::Accessor zoomAcsr;
        Zoom::Ruler mRuler {zoomAcsr, Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mScrollBar {zoomAcsr, Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, true, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
