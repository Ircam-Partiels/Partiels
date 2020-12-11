#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tooltip
{
    using Client = juce::TooltipClient;
    
    class Server
    : private juce::Timer
    {
    public:
        Server();
        ~Server() override = default;
        
        std::function<void(juce::String const&)> onChanged = nullptr;
        
    private:
        
        // juce::Timer
        void timerCallback() override;
        
        juce::String mTip;
    };
    
    class Display
    : public juce::Component
    {
    public:
        
        Display();
        ~Display() override = default;
        
        // juce::Component
        void resized() override;
        
    private:
        
        juce::Label mLabel;
        Server mServer;
    };
}

ANALYSE_FILE_END
