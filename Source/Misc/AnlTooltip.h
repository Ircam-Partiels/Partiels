#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

namespace Tooltip
{
    using Client = juce::TooltipClient;
    
    class Server
    : public juce::Timer
    {
    public:
        Server() = default;
        ~Server() override = default;
        
        std::function<void(juce::String const&)> onChanged = nullptr;
        
        // juce::Timer
        void timerCallback() override;
        
    private:
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
