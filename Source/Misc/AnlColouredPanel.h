#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

//! @brief A very basic class that is simple coloured component
class ColouredPanel
: public juce::Component
{
public:
    // clang-format off
    enum ColourIds : int
    {
          backgroundColourId = 0x2000100
    };
    // clang-format on

    ColouredPanel() = default;
    ~ColouredPanel() override = default;

    // juce::Component
    void paint(juce::Graphics& g) override;
};

ANALYSE_FILE_END
