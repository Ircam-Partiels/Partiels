#pragma once

#include "AnlTrackPlot.h"
#include "AnlTrackRuler.h"
#include "AnlTrackSnapshot.h"
#include "AnlTrackThumbnail.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Section
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2030000
        };
        // clang-format on

        Section(Director& director, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr);
        ~Section() override;

        juce::Rectangle<int> getPlotBounds() const;
        juce::String getIdentifier() const;
        void setResizable(bool state);

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

        Thumbnail mThumbnail{mDirector, mTimeZoomAccessor, mTransportAccessor};
        Decorator mThumbnailDecoration{mThumbnail};

        Snapshot mSnapshot{mAccessor, mTimeZoomAccessor, mTransportAccessor};
        Snapshot::Overlay mSnapshotOverlay{mSnapshot};
        Decorator mSnapshotDecoration{mSnapshotOverlay};

        Plot mPlot{mDirector, mTimeZoomAccessor, mTransportAccessor};
        Plot::Overlay mPlotOverlay{mPlot};
        Decorator mPlotDecoration{mPlotOverlay};

        Ruler mRuler{mAccessor};
        Decorator mRulerDecoration{mRuler};
        ScrollBar mScrollBar{mAccessor};

        ResizerBar mResizerBar{ResizerBar::Orientation::horizontal, true, {23, 2000}};
        ScrollHelper mScrollHelper;
    };
} // namespace Track

ANALYSE_FILE_END
