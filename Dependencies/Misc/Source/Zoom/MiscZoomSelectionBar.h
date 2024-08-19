#pragma once

#include "MiscZoomModel.h"

MISC_FILE_BEGIN

namespace Zoom
{
    class SelectionBar
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2010100
            , thumbCoulourId
        };

        enum class Anchor : bool
        {
              start
            , end
        };
        // clang-format on

        SelectionBar(Accessor& zoomAcsr);
        ~SelectionBar() override;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void colourChanged() override;

        bool getState() const;
        void setState(bool active);

        void setDefaultMouseCursor(juce::MouseCursor const& cursor);

        std::tuple<juce::Range<double>, Anchor> getRange() const;
        void setRange(juce::Range<double> const& range, Anchor anchor, juce::NotificationType notification);

        void setMarkers(std::set<double> const& markers);

        std::function<void(juce::Range<double> const&, Anchor)> onRangeChanged = nullptr;
        std::function<void(double const& value)> onMouseDown = nullptr;
        std::function<void(double const& value)> onMouseUp = nullptr;
        std::function<void(double const& value)> onDoubleClick = nullptr;

    private:
        void repaintRange();
        virtual void changed();

        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        juce::Range<double> mRange;
        bool mActive{true};
        double mCickPosition = 0.0;
        Anchor mAnchor{Anchor::start};
        std::set<double> mMarkers;
        juce::MouseCursor mDefaultMouseCursor{juce::MouseCursor::NormalCursor};
    };
} // namespace Zoom

MISC_FILE_END
