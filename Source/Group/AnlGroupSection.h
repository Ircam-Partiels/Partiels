#pragma once

#include "../Transport/AnlTransportPlayheadBar.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomScrollBar.h"
#include "AnlGroupPlot.h"
#include "AnlGroupSnapshot.h"
#include "AnlGroupThumbnail.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Section
    : public juce::Component
    , public juce::DragAndDropTarget
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2040000
            , highlightedColourId
        };
        // clang-format on

        Section(Director& director, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr);
        ~Section() override;

        std::function<void(juce::String const& identifier, bool copy)> onTrackInserted = nullptr;
        juce::Rectangle<int> getPlotBounds() const;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        void focusOfChildComponentChanged(juce::Component::FocusChangeType cause) override;

    private:
        // juce::DragAndDropTarget
        bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Transport::Accessor& mTransportAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

        Thumbnail mThumbnail{mDirector};
        Decorator mThumbnailDecoration{mThumbnail};

        Snapshot mSnapshot{mAccessor, mTransportAccessor, mTimeZoomAccessor};
        Snapshot::Overlay mSnapshotOverlay{mSnapshot};
        Decorator mSnapshotDecoration{mSnapshotOverlay};

        Plot mPlot{mAccessor, mTransportAccessor, mTimeZoomAccessor};
        Plot::Overlay mPlotOverlay{mPlot};
        Decorator mPlotDecoration{mPlotOverlay};

        Zoom::Ruler mRuler{mAccessor.getAcsr<AcsrType::zoom>(), Zoom::Ruler::Orientation::vertical};
        Zoom::ScrollBar mScrollBar{mAccessor.getAcsr<AcsrType::zoom>(), Zoom::ScrollBar::Orientation::vertical, true};

        ResizerBar mResizerBar{ResizerBar::Orientation::horizontal, true, {23, 2000}};
        bool mIsItemDragged{false};
    };
} // namespace Group

ANALYSE_FILE_END
