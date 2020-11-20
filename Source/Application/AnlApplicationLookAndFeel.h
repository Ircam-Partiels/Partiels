#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        ~LookAndFeel() override = default;
        
        // ScrollBar::LookAndFeelMethods
        bool areScrollbarButtonsVisible() override;
        
        // juce::ComboBox::LookAndFeelMethods
        void drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    private:
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LookAndFeel)
    };
}

ANALYSE_FILE_END

