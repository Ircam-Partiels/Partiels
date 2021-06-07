#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class Decorator
: public juce::Component
{
public:
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000300
        , normalBorderColourId
        , highlightedBorderColourId
    };
    // clang-format on

    Decorator(juce::Component& content, int borderThickness = 1, float cornerSize = 2.0f);
    ~Decorator() override = default;

    void setHighlighted(bool state);

    // juce::Component
    void resized() override;
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void lookAndFeelChanged() override;
    
private:
    juce::Component& mContent;
    int const mBorderThickness;
    float const mCornerSize;
    bool mIsHighlighted{false};
};

ANALYSE_FILE_END
