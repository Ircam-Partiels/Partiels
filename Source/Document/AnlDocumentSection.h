#pragma once

#include "../Group/AnlGroupStretchableSection.h"
#include "AnlDocumentHeader.h"
#include "AnlDocumentSelection.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Section
    : public juce::Component
    , public juce::DragAndDropContainer
    , private juce::ApplicationCommandManagerListener
    , private juce::AsyncUpdater
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2050000
        };
        // clang-format on

        Section(Director& director, juce::ApplicationCommandManager& commandManager);
        ~Section() override;

        juce::Rectangle<int> getPlotBounds(juce::String const& identifier) const;

        juce::TextButton pluginListButton;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

        static juce::BorderSize<int> getMainSectionBorderSize();

    private:
        // juce::DragAndDropContainer
        void dragOperationEnded(juce::DragAndDropTarget::SourceDetails const& details) override;

        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;

        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        void updateLayout();
        void updateExpandState();
        void updateFocus();
        void updateAutoresize();

        void moveTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier);
        void copyTrackToGroup(Group::Director& groupDirector, size_t index, juce::String const& trackIdentifier);

        Selection::Item getSelectionItem(juce::Component* component, juce::MouseEvent const& event) const;

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
        juce::ApplicationCommandManager& mApplicationCommandManager;
        Accessor::Listener mListener{typeid(*this).name()};
        Accessor::Receiver mReceiver;
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        Header mHeader;
        Icon mGridButton;
        Icon mExpandLayoutButton;
        Icon mResizeLayoutButton;
        Icon mMagnetizeButton;

        Zoom::Ruler mTimeRuler;
        Decorator mTimeRulerDecoration{mTimeRuler};
        Transport::LoopBar mLoopBar{mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>()};
        Decorator mLoopBarDecoration{mLoopBar};
        ColouredPanel mTopSeparator;

        std::map<juce::String, std::unique_ptr<Group::StretchableSection>> mGroupSections;
        DraggableTable mDraggableTable{"Group"};
        Viewport mViewport;
        ColouredPanel mBottomSeparator;
        Zoom::ScrollBar mTimeScrollBar{mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::ScrollBar::Orientation::horizontal};
        juce::TextButton mAddGroupButton;

        Tooltip::BubbleWindow mToolTipBubbleWindow;
        Selection::Item mLastSelectedItem;

        LayoutNotifier mLayoutNotifier;
        LayoutNotifier mExpandedNotifier;
        LayoutNotifier mFocusNotifier;
        bool mIsLoading = true;
    };
} // namespace Document

ANALYSE_FILE_END
