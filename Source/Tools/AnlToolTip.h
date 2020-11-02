#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

class ToolTipServer
: private juce::Timer
{
public:
    ToolTipServer();
    ~ToolTipServer() override = default;
    
    std::function<void(juce::String const&)> onToolTipChanged = nullptr;
    
private:
    
    // juce::Timer
    void timerCallback() override;
    
    juce::String mTip;
};

class ToolTipDisplay
: public juce::Component
{
public:
    
    ToolTipDisplay();
    ~ToolTipDisplay() override = default;
    
    // juce::Component
    void resized() override;
    
private:
    
    juce::Label mLabel;
    ToolTipServer mServer;
};

ANALYSE_FILE_END
