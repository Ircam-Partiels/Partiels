#pragma once

#include "../Group/AnlGroupStrechableSection.h"
#include "AnlDocumentReaderLayoutPanel.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Section
    : public juce::Component
    , public juce::DragAndDropContainer
    , private juce::FocusChangeListener
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2050000
        };
        // clang-format on

        Section(Director& director);
        ~Section() override;

        void moveKeyboardFocusTo(juce::String const& identifier);
        void showBubbleInfo(bool state);
        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;

        juce::ImageButton tooltipButton;
        std::function<void(void)> onSaveButtonClicked = nullptr;
        std::function<void(void)> onNewTrackButtonClicked = nullptr;
        std::function<void(void)> onNewGroupButtonClicked = nullptr;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    private:
        // juce::FocusChangeListener
        void globalFocusChanged(juce::Component* focusedComponent) override;

        void updateLayout();
        void updateHeights(bool force = false);
        void updateExpandState();
        void resizeHeader(juce::Rectangle<int>& bounds);
        void moveTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier);
        void copyTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier);

        class Viewport
        : public juce::Viewport
        {
        public:
            using juce::Viewport::Viewport;
            ~Viewport() override = default;

            void mouseWheelMove(juce::MouseEvent const& e, juce::MouseWheelDetails const& wheel) override;
            void visibleAreaChanged(juce::Rectangle<int> const& newVisibleArea) override;
            std::function<void(juce::Rectangle<int> const&)> onVisibleAreaChanged = nullptr;
        };

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Accessor::Receiver mReceiver;

        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Transport::Display mTransportDisplay{mAccessor.getAcsr<AcsrType::transport>()};

        ReaderLayoutPanel mReaderLayoutPanel{mDirector};
        juce::ImageButton mReaderLayoutButton;
        juce::String mReaderAlertMessage;
        juce::TextButton mDocumentName;
        juce::ImageButton mGridButton;
        juce::ImageButton mExpandLayoutButton;
        juce::ImageButton mResizeLayoutButton;
        juce::ImageButton mMagnetizeButton;

        Zoom::Ruler mTimeRuler;
        Decorator mTimeRulerDecoration{mTimeRuler};
        Transport::LoopBar mLoopBar{mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>()};
        Transport::PlayheadBar mPlayheadBar{mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>()};
        Decorator mLoopBarDecoration{mLoopBar};
        ColouredPanel mTopSeparator;

        std::map<juce::String, std::unique_ptr<Group::StrechableSection>> mGroupSections;
        std::vector<std::unique_ptr<Group::Accessor::SmartListener>> mGroupListeners;
        DraggableTable mDraggableTable{"Group"};
        Viewport mViewport;
        ColouredPanel mBottomSeparator;
        Zoom::ScrollBar mTimeScrollBar{mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::ScrollBar::Orientation::horizontal};
        juce::TextButton mAddButton;

        Tooltip::BubbleWindow mToolTipBubbleWindow;
        juce::Component* mFocusComponent{nullptr};
        ScrollHelper mScrollHelper;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Document

ANALYSE_FILE_END
