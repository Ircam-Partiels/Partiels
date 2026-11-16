#pragma once

#include "AnlGroupPropertyPanel.h"
#include "AnlGroupSlice.h"
#include "AnlGroupStateButton.h"

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
            , backgroundColourId
        };
        // clang-format on

        Thumbnail(Director& director, Transport::Accessor& transportAcsr);
        ~Thumbnail() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;

    private:
        //! @brief Shows the menu with the name of the group and the actions to show its windows and the windows of its tracks
        void showMenu();
        void fillPropertiesMenu(juce::PopupMenu& menu);
        void fillResultsMenu(juce::PopupMenu& menu);

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Accessor::Receiver mReceiver;

        PropertyPanel mPropertyPanel{mDirector};
        PropertyPanel::WindowContainer mPropertyWindowContainer{mPropertyPanel};
        Slice mSlice;
        Slice::WindowContainer mSliceWindowContainer{mSlice};
        Icon mPropertiesButton;
        Icon mResultsButton;
        Icon mSliceButton;
        StateButton mStateButton{mDirector};
        Icon mExpandButton;
        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
