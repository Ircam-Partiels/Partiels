#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupSection.h"
#include "AnlDocumentFileInfoPanel.h"
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
        
        Section(Accessor& accessor, juce::AudioFormatManager& audioFormatManager);
        ~Section() override;
        
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void colourChanged() override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        
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
            void paint(juce::Graphics& g) override;
            
        private:
            Accessor& mAccessor;
            Accessor::Listener mListener;
            
            GroupSection mGroupSection {mAccessor};
            std::vector<std::unique_ptr<Track::Section>> mSections;
            DraggableTable mDraggableTable;
            ConcertinaTable mConcertinaTable {"", false};
            BoundsListener mBoundsListener;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        FileInfoPanel mFileInfoPanel;
        FileInfoButton mFileInfoButton {mFileInfoPanel};
        Decorator mFileInfoButtonDecoration {mFileInfoButton, 1, 2.0f};
        
        Zoom::Ruler mTimeRuler {mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::Ruler::Orientation::horizontal};
        Decorator mTimeRulerDecoration {mTimeRuler, 1, 2.0f};
        juce::Component mLoopRuler;
        Decorator mLoopRulerDecoration {mLoopRuler, 1, 2.0f};
        
        GroupContainer mGroupContainer {mAccessor};
        juce::Viewport mViewport;
        Zoom::ScrollBar mTimeScrollBar {mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::ScrollBar::Orientation::horizontal};
    };
}

ANALYSE_FILE_END
