#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class Decorator
: public juce::Component
{
public:
    
    enum ColourIds : int
    {
          backgroundColourId = 0x2000200
        , borderColourId
    };
    
    Decorator(juce::Component& content, int borderThickness, float cornerSize);
    ~Decorator() override = default;
    
    // juce::Component
    void resized() override;
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    
private:
    juce::Component& mContent;
    int const mBorderThickness;
    float const  mCornerSize;
};

ANALYSE_FILE_END
