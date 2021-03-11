#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class LoadingCircle
: public juce::Component
, public juce::SettableTooltipClient
, private juce::Timer
{
public:
    
    enum ColourIds : int
    {
          backgroundColourId = 0x2000400
        , foregroundColourId
    };
    
    LoadingCircle(juce::Image const& inactiveImage = {});
    ~LoadingCircle() override = default;
    
    void setActive(bool state);
    void setInactiveImage(juce::Image const& inactiveImage);
    
    // juce::Component
    void paint(juce::Graphics& g) override;
    
private:
    
    // juce::Timer
    void timerCallback() override;
    
    juce::Image mImageActive;
    juce::Image mImageInactive;
    float mRotation = 0.0f;
};

ANALYSE_FILE_END
