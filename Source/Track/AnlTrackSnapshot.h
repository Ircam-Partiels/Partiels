#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Snapshot
    : public juce::Component
    {
    public:
        Snapshot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor);
        ~Snapshot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public juce::Component
        , public Tooltip::BubbleClient
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
            Accessor::Listener mListener;
        };

    private:
        static void paintPoints(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        static void paintColumns(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Zoom::Accessor::Listener mValueZoomListener;
        Zoom::Accessor::Listener mBinZoomListener;
        Accessor::Listener mListener;
    };
} // namespace Track

ANALYSE_FILE_END
