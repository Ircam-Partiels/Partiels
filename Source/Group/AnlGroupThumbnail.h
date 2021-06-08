#pragma once

#include "AnlGroupButtonState.h"
#include "AnlGroupPropertyPanel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Thumbnail
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              textColourId = 0x2040100
            , titleBackgroundColourId
        };
        // clang-format on

        Thumbnail(Director& director);
        ~Thumbnail() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;

        PropertyPanel mPropertyPanel{mDirector};
        juce::ImageButton mDropdownButton;
        juce::ImageButton mPropertiesButton;
        StateButton mStateButton{mAccessor};
        juce::ImageButton mExpandButton;
    };
} // namespace Group

ANALYSE_FILE_END
