#pragma once

#include "AnlApplicationController.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        ~LookAndFeel() override = default;
        
    private:
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LookAndFeel)
    };
}

ANALYSE_FILE_END

