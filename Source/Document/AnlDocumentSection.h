#pragma once

#include "AnlDocumentModel.h"

#include "../Layout/AnlLayout.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Analyzer/AnlAnalyzerSection.h"

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
        
        Section(Accessor& accessor);
        ~Section() override;
        
        std::function<void(juce::String const& identifier)> onRemoveAnalyzer = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        ResizerBar mResizerBar {ResizerBar::Orientation::vertical, {50, 300}};
        Zoom::Ruler mZoomTimeRuler {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal};
        std::vector<std::unique_ptr<Analyzer::Section>> mSections;
        ConcertinaPanel mConcertinalPanel {"", false};
        juce::Viewport mViewport;
        Zoom::ScrollBar mZoomTimeScrollBar {mAccessor.getAccessor<AcsrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal};
    };
}

ANALYSE_FILE_END
