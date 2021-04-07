#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupSection.h"
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
              backgroundColourId = 0x2040200
        };
        
        Section(Accessor& accessor);
        ~Section() override;
        
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        class GroupContainer
        : public juce::Component
        {
        public:
            GroupContainer(Accessor& accessor);
            ~GroupContainer() override;
            
            std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
            
            // juce::Component
            void resized() override;
        private:
            Accessor& mAccessor;
            Accessor::Listener mListener;
            
            ResizerBar mResizerBar {ResizerBar::Orientation::vertical, true, {50, 300}};
            GroupSection mGroupSection {mAccessor, mResizerBar};
            std::vector<std::unique_ptr<Track::Section>> mSections;
            DraggableTable mDraggableTable;
            ConcertinaTable mConcertinaTable {"", false};
            BoundsListener mBoundsListener;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::Ruler::Orientation::horizontal};
        
        GroupContainer mGroupContainer {mAccessor};
        juce::Viewport mViewport;
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::ScrollBar::Orientation::horizontal};
    };
}

ANALYSE_FILE_END
