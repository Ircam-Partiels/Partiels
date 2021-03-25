#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupThumbnail.h"
#include "AnlDocumentGroupSnapshot.h"
#include "AnlDocumentGroupPlot.h"
#include "../Track/AnlTrackSection.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Section
    : public juce::Component
    , public juce::DragAndDropContainer
    {
    public:
        enum ColourIds : int
        {
              backgroundColourId = 0x2040300
        };
        
        Section(Accessor& accessor);
        ~Section() override;
        
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;
        
        juce::Component mPlayheadContainer;
        Zoom::Playhead mPlayhead {mAccessor.getAccessor<AcsrType::timeZoom>(0)};
        
        ResizerBar mResizerBar {ResizerBar::Orientation::vertical, true, {50, 300}};
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal};
        
        GroupThumbnail mThumbnail {mAccessor};
        Decorator mThumbnailDecorator {mThumbnail, 1, 4.0f};
        
        GroupSnapshot mSnapshot {mAccessor};
        Decorator mSnapshotDecorator {mSnapshot, 1, 4.0f};
        
        GroupPlot mPlot {mAccessor};
        Decorator mPlotDecorator {mPlot, 1, 4.0f};
        
        std::vector<std::unique_ptr<Track::Section>> mSections;
        DraggableTable mDraggableTable;
        ConcertinaTable mConcertinaTable {"", false};
        juce::Viewport mViewport;
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal};
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, true, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
