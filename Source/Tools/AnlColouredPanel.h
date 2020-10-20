#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    //! @brief A very basic class that is simple coloured component
    class ColouredPanel
    : public juce::Component
    {
    public:

        enum ColourIds : int
        {
            backgroundColourId = 0x2000000
        };
        
        ColouredPanel() = default;
        ~ColouredPanel() override = default;
        
        // juce::Component
        void paint(juce::Graphics& g) override;
    };
}

ANALYSE_FILE_END
