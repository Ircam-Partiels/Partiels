#pragma once

#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlTrackPlot.h"
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

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void colourChanged() override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        void focusOfChildComponentChanged(juce::Component::FocusChangeType cause) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener;

        Thumbnail mThumbnail{mAccessor};
        Decorator mThumbnailDecoration{mThumbnail, 1, 2.0f};

        Snapshot mSnapshot{mAccessor, mTimeZoomAccessor};
        Snapshot::Overlay mSnapshotOverlay{mSnapshot};
        Decorator mSnapshotDecoration{mSnapshotOverlay, 1, 2.0f};

        Plot mPlot{mAccessor, mTimeZoomAccessor, mTransportAccessor};
        Plot::Overlay mPlotOverlay{mPlot};
        Decorator mPlotDecoration{mPlotOverlay, 1, 2.0f};

        Zoom::Ruler mValueRuler{mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mValueScrollBar{mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::Ruler mBinRuler{mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mBinScrollBar{mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBar{ResizerBar::Orientation::horizontal, true, {23, 2000}};
    };
} // namespace Track

ANALYSE_FILE_END
