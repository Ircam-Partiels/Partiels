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
              backgroundColourId = 0x2000400
            , sectionColourId = 0x2000401
        };
        
        Section(Accessor& accessor);
        ~Section() override;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        class Content
        : public juce::Component
        {
        public:
            
            Content(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr);
            ~Content() override = default;
            
            std::function<void(void)> onAnalyse = nullptr;
            std::function<void(void)> onRemove = nullptr;
            std::function<void(int)> onThumbnailResized = nullptr;
            
            void setTime(double time);
            void setThumbnailSize(int size);
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
        private:
            
            Analyzer::Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor& mValueZoomAccessor {mAccessor.getAccessor<Analyzer::AcsrType::zoom>(0)};
            
            Analyzer::Thumbnail mThumbnail {mAccessor};
            Analyzer::InstantRenderer mInstantRenderer {mAccessor, mTimeZoomAccessor};
            Zoom::Ruler mRuler {mValueZoomAccessor, Zoom::Ruler::Orientation::vertical};
            Layout::ResizerBar mResizerBar {Layout::ResizerBar::Orientation::vertical};
            Analyzer::TimeRenderer mTimeRenderer {mAccessor, mTimeZoomAccessor};
            Zoom::ScrollBar mScrollbar {mValueZoomAccessor, Zoom::ScrollBar::Orientation::vertical, true};
        };
        
        struct Container
        {
        public:
            Container(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr)
            : content(acsr, timeZoomAcsr)
            , accessor(acsr)
            {
            }
            
            Content content;
            Analyzer::Accessor& accessor;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal};
        Playhead mPlayhead {mAccessor};
        std::vector<std::unique_ptr<Container>> mContents;
        Layout::StrechableContainer::Section mContainer {mAccessor.getAccessor<AcsrType::layout>(0)};
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

ANALYSE_FILE_END
