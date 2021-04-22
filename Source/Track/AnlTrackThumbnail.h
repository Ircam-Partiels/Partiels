#pragma once

#include "AnlTrackPropertyPanel.h"
#include "AnlTrackStateButton.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Thumbnail
    : public juce::Component
    , public juce::SettableTooltipClient
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              textColourId = 0x2030100
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
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;

        PropertyPanel mPropertyPanel{mAccessor};
        juce::ImageButton mDropdownButton;
        juce::ImageButton mPropertiesButton;
        juce::ImageButton mExportButton;
        StateButton mStateButton{mAccessor};
    };
} // namespace Track

ANALYSE_FILE_END
