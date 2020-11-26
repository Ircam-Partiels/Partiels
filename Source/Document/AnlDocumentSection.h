#pragma once

#include "AnlDocumentModel.h"

#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Analyzer/AnlAnalyzerThumbnail.h"
#include "../Analyzer/AnlAnalyzerResultRenderer.h"
#include "../Analyzer/AnlAnalyzerProcessor.h"

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
        };
        
        Section(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager);
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
            
            Analyzer::Accessor const& getAccessor() const;
            
            std::function<void(void)> onAnalyse = nullptr;
            std::function<void(void)> onRemove = nullptr;
            std::function<void(void)> onRelaunch = nullptr;
            
            // juce::Component
            void resized() override;
            
        private:
            
            Analyzer::Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor& mValueZoomAccessor {mAccessor.getAccessor<Analyzer::AttrType::zoom>(0)};
            Analyzer::Thumbnail mThumbnail {mAccessor};
            Analyzer::ResultRenderer mRenderer {mAccessor, mTimeZoomAccessor};
            Zoom::Ruler mRuler {mValueZoomAccessor, Zoom::Ruler::Orientation::vertical};
            Zoom::ScrollBar mScrollbar {mValueZoomAccessor, Zoom::ScrollBar::Orientation::vertical, true};
        };
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        std::vector<std::unique_ptr<Content>> mContents;
        Layout::StrechableContainer::Section mContainer {mAccessor.getAccessor<AttrType::layout>(0)};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
    };
}

ANALYSE_FILE_END
