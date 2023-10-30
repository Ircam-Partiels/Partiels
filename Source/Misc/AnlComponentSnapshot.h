#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class ComponentSnapshot
: public juce::Component
, public juce::Timer
{
public:
    ComponentSnapshot() = default;
    ~ComponentSnapshot() override = default;

    void takeSnapshot(juce::Component& component, juce::String const& name, juce::Colour const& backgroundColour);

    // juce::Component
    void paintOverChildren(juce::Graphics& g) override;

    static juce::MouseCursor getCameraCursor();

private:
    // juce::Timer
    void timerCallback() override;

    float mAlpha = 0.0f;
    juce::Image mImage;
};

ANALYSE_FILE_END
