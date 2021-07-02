#pragma once

#include "AnlTransportModel.h"

ANALYSE_FILE_BEGIN

namespace Transport
{
    class Display
    : public juce::Component
    {
    public:
        Display(Accessor& accessor);
        ~Display() override;

        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;

        void setMaxTime(double timeInSeconds);

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};

        juce::ImageButton mRewindButton;
        juce::ImageButton mPlaybackButton;
        juce::ImageButton mLoopButton;

        HMSmsField mPosition;
        juce::Slider mVolumeSlider{juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::NoTextBox};
    };
} // namespace Transport

ANALYSE_FILE_END
