#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Transport
    : public juce::Component
    {
    public:
        Transport(Accessor& accessor);
        ~Transport() override;
        
        // juce::Component
        void resized() override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;

        juce::ImageButton mRewindButton;
        juce::ImageButton mPlaybackButton;
        juce::ImageButton mLoopButton;
        
        HMSmsField mPosition;
        juce::Slider mVolumeSlider {juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::NoTextBox};
    };
}

ANALYSE_FILE_END
