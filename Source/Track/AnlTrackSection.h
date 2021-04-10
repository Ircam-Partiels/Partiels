#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlTrackThumbnail.h"
#include "AnlTrackPlot.h"
#include "AnlTrackSnapshot.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Section
    : public juce::Component
    {
    public:
        enum ColourIds : int
        {
              sectionColourId = 0x2030000
        };
        
        Section(Accessor& accessor, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr);
        ~Section() override;
        
        juce::String getIdentifier() const;
        
        std::function<void(void)> onRemove = nullptr;
        
        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        
    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener;
        
        Thumbnail mThumbnail {mAccessor};
        Decorator mThumbnailDecoration {mThumbnail, 1, 2.0f};
        
        Snapshot mSnapshot {mAccessor, mTimeZoomAccessor};
        Snapshot::Overlay mSnapshotOverlay {mSnapshot};
        Decorator mSnapshotDecoration {mSnapshotOverlay, 1, 2.0f};
        
        Plot mPlot {mAccessor, mTimeZoomAccessor, mTransportAccessor};
        Plot::Overlay mPlotOverlay {mPlot};
        Decorator mPlotDecoration {mPlotOverlay, 1, 2.0f};
        
        Zoom::Ruler mValueRuler {mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mValueScrollBar {mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::Ruler mBinRuler  {mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mBinScrollBar {mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        
        ResizerBar mResizerBar {ResizerBar::Orientation::horizontal, true, {50, 2000}};
    };
}

ANALYSE_FILE_END
