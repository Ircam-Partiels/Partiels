#pragma once

#include "../Transport/AnlTransportModel.h"
#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Snapshot
    : public juce::Component
    {
    public:
        Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Snapshot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public juce::Component
        , public Tooltip::BubbleClient
        , public juce::SettableTooltipClient
        {
        public:
            Overlay(Snapshot& snapshot);
            ~Overlay() override;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;

        private:
            void updateTooltip(juce::Point<int> const& pt);

            Snapshot& mSnapshot;
            Accessor& mAccessor;
            Transport::Accessor& mTransportAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
        };

        static void paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, double time, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour);

    private:
        static void paintGrid(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour);
        static void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr, double time);
        static void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr, double time);

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
        Transport::Accessor::Listener mTransportListener{typeid(*this).name()};
    };
} // namespace Track

ANALYSE_FILE_END
