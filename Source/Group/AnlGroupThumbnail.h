#pragma once

#include "AnlGroupPropertyPanel.h"
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

        Thumbnail(Director& director);
        ~Thumbnail() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        Accessor::Receiver mReceiver;

        PropertyPanel mPropertyPanel{mDirector};
        PropertyPanel::WindowContainer mPropertyWindowContainer{mPropertyPanel};
        Icon mPropertiesButton{Icon::Type::properties};
        Icon mEditButton{Icon::Type::edit};
        StateButton mStateButton{mDirector};
        Icon mExpandButton{Icon::Type::expand};
    };
} // namespace Group

ANALYSE_FILE_END
