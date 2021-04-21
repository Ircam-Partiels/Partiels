#pragma once

#include "../Group/AnlGroupStrechableSection.h"
#include "../Transport/AnlTransportLoopBar.h"
#include "../Transport/AnlTransportPlayheadBar.h"
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

        Section(Accessor& accessor, juce::AudioFormatManager& audioFormatManager);
        ~Section() override;

        std::function<void(juce::String const& groupIdentifier, juce::String const& trackIdentifier)> onTrackInserted = nullptr;

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

        Accessor& mAccessor;
        Accessor::Listener mListener;

        FileInfoPanel mFileInfoPanel;
        FileInfoButton mFileInfoButton{mFileInfoPanel};
        Decorator mFileInfoButtonDecoration{mFileInfoButton, 1, 2.0f};
        juce::ImageButton mTooltipButton{"Document::Section::TooltipButton"};

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
