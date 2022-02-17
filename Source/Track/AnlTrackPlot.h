#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Plot
    : public juce::Component
    {
    public:
        Plot(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~Plot() override;

        // juce::Component
        void paint(juce::Graphics& g) override;

        class Overlay
        : public ComponentSnapshot
        , public Tooltip::BubbleClient
        , public juce::SettableTooltipClient
        {
        public:
            Overlay(Plot& plot);
            ~Overlay() override;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void mouseDrag(juce::MouseEvent const& event) override;
            void mouseUp(juce::MouseEvent const& event) override;

        private:
            void updateTooltip(juce::Point<int> const& pt);
            void updateMode(juce::MouseEvent const& event);

            Plot& mPlot;
            Accessor& mAccessor;
            Zoom::Accessor& mTimeZoomAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
            Transport::PlayheadBar mTransportPlayheadBar;
            bool mSnapshotMode{false};
        };

        static void paint(Accessor const& accessor, Zoom::Accessor const& timeZoomAcsr, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const colour);

    private:
        static void paintGrid(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour const colour);
        static void paintMarkers(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        static void paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        static void paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);

        class PathArrangement
        {
        public:
            void stopLine();
            void addLine(float x1, float x2, float y);
            void draw(juce::Graphics& g, juce::Colour const& foreground, juce::Colour const& shadow);

        private:
            juce::Path mPath;
            juce::Point<float> mLastPoint;
            bool mShouldStartNewPath{true};
        };

        class LabelArrangement
        {
        public:
            LabelArrangement(juce::Font const& font, juce::String const unit, int numDecimals);
            ~LabelArrangement() = default;

            void addValue(float value, float x, float y);
            void draw(juce::Graphics& g, juce::Colour const& colour);

        private:
            auto static constexpr invalid_value = std::numeric_limits<float>::max();
            auto static constexpr epsilon = std::numeric_limits<float>::epsilon();

            struct LabelInfo
            {
                LabelInfo(float o)
                : offset(o)
                {
                }

                float const offset;
                juce::String text;
                float x;
                float y;
                float w;

                inline bool operator==(LabelInfo const& rhs) const
                {
                    return text == rhs.text && std::abs(x - rhs.x) < epsilon && std::abs(y - rhs.y) < epsilon;
                }
            };

            juce::Font const& mFont;
            juce::String const mUnit;
            int const mNumDecimal;
            float const mCharWidth{mFont.getStringWidthFloat("0")};
            float const mUnitWidth{mFont.getStringWidthFloat(mUnit)};
            LabelInfo mLowInfo{2.0f + mFont.getAscent()};
            LabelInfo mHighInfo{2.0f - mFont.getDescent()};
            float mLastValue = invalid_value;
            juce::GlyphArrangement mGlyphArrangement;
        };

        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};
        Zoom::Grid::Accessor::Listener mGridListener{typeid(*this).name()};
    };
} // namespace Track

ANALYSE_FILE_END
