#pragma once

#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Ruler
    : public juce::Component
    {
    public:
        Ruler(Accessor& accessor);
        ~Ruler() override;

        // juce::Component
        void resized() override;
        void paint(juce::Graphics& g) override;
        void colourChanged() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Zoom::Ruler>> mRulers;
        FrameType mFrameType{Track::FrameType::value};
    };

    class ScrollBar
    : public juce::Component
    {
    public:
        ScrollBar(Accessor& accessor);
        ~ScrollBar() override;

        // juce::Component
        void resized() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        Zoom::ScrollBar mValueScrollBar{mAccessor.getAcsr<AcsrType::valueZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
        Zoom::ScrollBar mBinScrollBar{mAccessor.getAcsr<AcsrType::binZoom>(), Zoom::ScrollBar::Orientation::vertical, true};
    };

    class SelectionBar
    : public juce::Component
    {
    public:
        SelectionBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor);
        ~SelectionBar() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;

    private:
        Accessor& mAccessor;
        Zoom::Accessor& mTimeZoomAccessor;
        Transport::Accessor& mTransportAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        std::vector<std::unique_ptr<Transport::SelectionBar>> mSelectionBars;
    };
} // namespace Track

ANALYSE_FILE_END
