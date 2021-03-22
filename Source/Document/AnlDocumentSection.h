#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentPlot.h"
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
              backgroundColourId = 0x2050100
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
        
        ResizerBar mResizerBar {ResizerBar::Orientation::vertical, {50, 300}};
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal};
        Plot mPlot {mAccessor};
        std::vector<std::unique_ptr<Track::Section>> mSections;
        DraggableTable mDraggableTable;
        juce::Viewport mViewport;
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal};
    };
}

ANALYSE_FILE_END
