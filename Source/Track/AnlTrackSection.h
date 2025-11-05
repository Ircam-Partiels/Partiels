#pragma once

#include "AnlTrackPlot.h"
#include "AnlTrackRuler.h"
#include "AnlTrackSnapshot.h"
#include "AnlTrackThumbnail.h"
#include "Editor/AnlTrackEditor.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Section
    : public juce::Component
    {
    public:
        using ResizerFn = std::function<void(juce::String const&, int)>;

        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2030000
        };
        // clang-format on

        Section(Director& director, juce::ApplicationCommandManager& commandManager, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr, PresetList::Accessor& presetListAcsr, ResizerFn resizerFn);
        ~Section() override;

        juce::Component const& getPlot() const;
        juce::String getIdentifier() const;
        void setResizable(bool state);

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;

        static constexpr int getThumbnailOffset() { return 24; }
        static constexpr int getThumbnailWidth() { return 24; }
        static constexpr int getSnapshotWidth() { return 36; }
        static constexpr int getScrollBarWidth() { return 8; }
        static constexpr int getRulerWidth() { return 16; }

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

        Thumbnail mThumbnail;
        Decorator mThumbnailDecoration{mThumbnail};

        Snapshot mSnapshot{mAccessor, mTimeZoomAccessor, mTransportAccessor};
        Snapshot::Overlay mSnapshotOverlay{mSnapshot};
        Decorator mSnapshotDecoration{mSnapshotOverlay};

        Plot mPlot;
        Editor mEditor;
        Decorator mPlotDecoration{mEditor};

        Ruler mRuler{mAccessor};
        Decorator mRulerDecoration{mRuler};
        ScrollBar mScrollBar{mAccessor};

        ResizerBar mResizerBar{ResizerBar::Orientation::horizontal, true, {23, 2000}};
    };
} // namespace Track

ANALYSE_FILE_END
