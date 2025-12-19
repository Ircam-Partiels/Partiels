#pragma once

#include "AnlGroupEditor.h"
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
        using ResizerFn = std::function<void(juce::String const&, int)>;

        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2040000
            , overflyColourId
        };
        // clang-format on

        Section(Director& director, juce::ApplicationCommandManager& commandManager, Transport::Accessor& transportAcsr, Zoom::Accessor& timeZoomAcsr, ResizerFn resizerFn);
        ~Section() override;

        std::function<void(juce::String const& identifier, bool copy)> onTrackInserted = nullptr;
        juce::Component const& getPlot() const;
        juce::String getIdentifier() const;
        void setResizable(bool state);

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;

    private:
        // juce::DragAndDropTarget
        bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;
        void itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails) override;

        void updateContent();
        void performMagnify(float magnifyAmount);

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

        Plot mPlot;
        Editor mEditor;
        Decorator mPlotDecoration{mEditor};

        std::unique_ptr<juce::Component> mRuler;
        std::unique_ptr<Decorator> mDecoratorRuler;
        std::unique_ptr<juce::Component> mScrollBar;
        juce::String mVerticalZoomIdentifier;

        ResizerBar mResizerBar{ResizerBar::Orientation::horizontal, true, {23, 2000}};
        bool mIsItemDragged{false};
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
