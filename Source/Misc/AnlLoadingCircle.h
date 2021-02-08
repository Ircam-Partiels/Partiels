#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class LoadingCircle
: public juce::Component
, private juce::Timer
{
public:
    
    enum ColourIds : int
    {
          backgroundColourId = 0x2006000
        , foregroundColourId = 0x2006001
    };
    
    LoadingCircle();
    ~LoadingCircle() override = default;
    
    void setActive(bool state);
    
    // juce::Component
    void paint(juce::Graphics& g) override;
    
private:
    
    // juce::Timer
    void timerCallback() override;
    
    juce::Image mImageProcessing;
    juce::Image mImageReady;
    float mRotation = 0.0f;
};

ANALYSE_FILE_END
