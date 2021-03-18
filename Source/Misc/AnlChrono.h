#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class Chrono
{
public:
    
    Chrono(juce::String const& domain, juce::String const& message);
    ~Chrono() = default;
    
    void start();
    void stop();
    
private:
    
    juce::String const mDomain;
    juce::String const mMessage;
    juce::int64 mTime;
};

ANALYSE_FILE_END
