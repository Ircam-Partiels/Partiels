#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AudioSettings
    : public juce::Component
    , private juce::ChangeListener
    {
    public:
        AudioSettings();
        ~AudioSettings() override;
        
        void show();
        
        // juce::Component
        void resized() override;
        
    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
        PropertyList mPropertyDriver;
        PropertyList mPropertyOutputDevice;
        PropertyList mPropertySampleRate;
#ifdef JUCE_MAC
        PropertyList mPropertyBufferSize;
        PropertyNumber mPropertyBufferSizeNumber;
#else

#endif
        PropertyTextButton mPropertyDriverPanel;
        
        FloatingWindow mFloatingWindow {"Audio Settings"};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettings)
    };
}

ANALYSE_FILE_END

