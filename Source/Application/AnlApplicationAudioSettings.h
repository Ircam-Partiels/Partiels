#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AudioSettings
    : public FloatingWindowContainer
    , private juce::ChangeListener
    {
    public:
        AudioSettings();
        ~AudioSettings() override;

        // juce::Component
        void resized() override;

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        PropertyList mPropertyDriver;
        PropertyList mPropertyOutputDevice;
        PropertyList mPropertySampleRate;
        PropertyList mPropertyBufferSize;
        PropertyNumber mPropertyBufferSizeNumber;
        PropertyTextButton mPropertyDriverPanel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettings)
    };
} // namespace Application

ANALYSE_FILE_END
