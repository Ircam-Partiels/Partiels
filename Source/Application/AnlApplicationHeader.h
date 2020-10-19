#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Header
    : public juce::Component
    , private juce::ApplicationCommandManagerListener
    {
    public:
        
        Header();
        ~Header() override = default;
        
        // juce::Component
        void resized() override;
        
    private:
        
        // juce::ApplicationCommandManagerListener
        void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
        void applicationCommandListChanged() override;
        
//        ilf::AudioTransportControls mAudioTransportControls;
//        ilf::HMSmsField mHMSmsTimeField;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Header)
    };
}

ANALYSE_FILE_END

