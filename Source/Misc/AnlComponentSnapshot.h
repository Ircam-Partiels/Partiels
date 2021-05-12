#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ComponentSnapshot
: public juce::Component
, public juce::Timer
{
public:
    ComponentSnapshot() = default;
    ~ComponentSnapshot() override = default;

    void showCameraCursor(bool state);
    void takeSnapshot(juce::Component& component, juce::String const& name);

    // juce::Component
    void paintOverChildren(juce::Graphics& g) override;

private:
    // juce::Timer
    void timerCallback() override;

    float mAlpha = 0.0f;
    juce::Image mImage;
};

ANALYSE_FILE_END
