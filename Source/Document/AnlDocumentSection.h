#pragma once

#include "../Group/AnlGroupStrechableSection.h"
#include "../Transport/AnlTransportDisplay.h"
#include "../Transport/AnlTransportLoopBar.h"
#include "../Transport/AnlTransportPlayheadBar.h"
#include "AnlDocumentDirector.h"
#include "AnlDocumentFileInfoPanel.h"
#include "AnlDocumentModel.h"

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

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void colourChanged() override;
        void lookAndFeelChanged() override;
        void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
        void mouseMagnify(juce::MouseEvent const& event, float magnifyAmount) override;
        juce::KeyboardFocusTraverser* createFocusTraverser() override;

    private:
        // juce::FocusChangeListener
        void globalFocusChanged(juce::Component* focusedComponent) override;

        void updateLayout();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener;

        Transport::Display mTransportDisplay{mAccessor.getAcsr<AcsrType::transport>()};

        FileInfoPanel mFileInfoPanel{mAccessor, mDirector.getAudioFormatManager()};
        juce::ImageButton mFileInfoButton;
        juce::Label mFileInfoLabel;

        Zoom::Ruler mTimeRuler{mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::Ruler::Orientation::horizontal};
        Decorator mTimeRulerDecoration{mTimeRuler, 1, 2.0f};
        Transport::LoopBar mLoopBar{mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>()};
        Transport::PlayheadBar mPlayheadBar{mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>()};
        Decorator mLoopBarDecoration{mLoopBar, 1, 2.0f};

        std::map<juce::String, std::unique_ptr<Group::StrechableSection>> mGroupSections;
        DraggableTable mDraggableTable{"Group"};
        juce::Viewport mViewport;
        Zoom::ScrollBar mTimeScrollBar{mAccessor.getAcsr<AcsrType::timeZoom>(), Zoom::ScrollBar::Orientation::horizontal};

        Tooltip::BubbleWindow mToolTipBubbleWindow;
        juce::Component* mFocusComponent{nullptr};
    };
} // namespace Document

ANALYSE_FILE_END
