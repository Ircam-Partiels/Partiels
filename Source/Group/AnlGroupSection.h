#pragma once

#include "AnlGroupPlot.h"
#include "../Analyzer/AnlAnalyzerSection.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Section
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
            sectionColourId = 0x20006100
        };
        
        Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, juce::Component& separator);
        ~Section() override;
        
        std::function<void(juce::String const& identifier)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;
        
        class Container
        : public juce::Component
        {
            
        };
        
//        Thumbnail mThumbnail {mAccessor};
//        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        Plot mPlot {mAccessor, mTimeZoomAccessor};
        std::vector<std::unique_ptr<Analyzer::Section>> mSubSections;
        DraggableTable mDraggableTable;
        juce::Viewport mViewport;
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, {50, 2000}};
    };
}

ANALYSE_FILE_END
