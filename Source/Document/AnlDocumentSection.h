#pragma once

#include "AnlDocumentModel.h"

#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Analyzer/AnlAnalyzerThumbnail.h"
#include "../Analyzer/AnlAnalyzerTimeRenderer.h"
#include "../Analyzer/AnlAnalyzerInstantRenderer.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"
#include "AnlDocumentPlayhead.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Section
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
            sectionColourId = 0x2000401
        };
        
        Section(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager);
        ~Section() override;
        
        // juce::Component
        void resized() override;
        
    private:
        
        class Content
        : public juce::Component
        {
        public:
            
            Content(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr, juce::StretchableLayoutManager& layoutManager);
            ~Content() override = default;
            
            std::function<void(void)> onAnalyse = nullptr;
            std::function<void(void)> onRemove = nullptr;
            std::function<void(int)> onThumbnailResized = nullptr;
            
            void setTime(double time);
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
        private:
            
            Analyzer::Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor& mValueZoomAccessor {mAccessor.getAccessor<Analyzer::AttrType::zoom>(0)};
            juce::StretchableLayoutManager& mLayoutManager;
            
            Analyzer::Thumbnail mThumbnail {mAccessor};
            Analyzer::InstantRenderer mInstantRenderer {mAccessor, mTimeZoomAccessor};
            Zoom::Ruler mRuler {mValueZoomAccessor, Zoom::Ruler::Orientation::vertical};
            Layout::StretchableResizerBar mResizerBar {&mLayoutManager, 3, true};
            Analyzer::TimeRenderer mTimeRenderer {mAccessor, mTimeZoomAccessor};
            Zoom::ScrollBar mScrollbar {mValueZoomAccessor, Zoom::ScrollBar::Orientation::vertical, true};
            juce::Component mDummy;
        };
        
        struct Container
        {
        public:
            Container(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr, juce::StretchableLayoutManager& layoutManager)
            : content(acsr, timeZoomAcsr, layoutManager)
            , accessor(acsr)
            {
            }
            
            Content content;
            Analyzer::Accessor& accessor;
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        
        juce::StretchableLayoutManager mLayoutManager;
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAccessor<AttrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal};
        Playhead mPlayhead {mAccessor};
        std::vector<std::unique_ptr<Container>> mContents;
        Layout::StrechableContainer::Section mContainer {mAccessor.getAccessor<AttrType::layout>(0)};
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAccessor<AttrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

ANALYSE_FILE_END
