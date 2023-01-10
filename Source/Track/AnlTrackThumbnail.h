#pragma once

#include "AnlTrackPropertyPanel.h"
#include "AnlTrackStateButton.h"
#include "Result/AnlTrackResultTable.h"

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
            , backgroundColourId
        };
        // clang-format on

        Thumbnail(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAcsr);
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
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;

        PropertyPanel mPropertyPanel{mDirector};
        PropertyPanel::WindowContainer mPropertyWindowContainer{mPropertyPanel};
        Result::Table mResultsTable{mDirector, mTimeZoomAccessor, mTransportAccessor};
        Result::Table::WindowContainer mResultsWindowContainer{mResultsTable};
        Icon mPropertiesButton{Icon::Type::properties};
        Icon mEditButton{Icon::Type::edit};
        StateButton mStateButton{mDirector};
    };
} // namespace Track

ANALYSE_FILE_END
