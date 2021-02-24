#pragma once

#include "AnlGroupPlot.h"

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
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        juce::Component& mSeparator;
        Accessor::Listener mListener;
        BoundsListener mBoundsListener;
        
//        Thumbnail mThumbnail {mAccessor};
//        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        Plot mPlot {mAccessor, mTimeZoomAccessor};
        
        ResizerBar mResizerBarLeft {ResizerBar::Orientation::horizontal, {50, 2000}};
        ResizerBar mResizerBarRight {ResizerBar::Orientation::horizontal, {50, 2000}};
    };
}

ANALYSE_FILE_END
