#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public Misc::LookAndFeel
    {
    public:
        LookAndFeel();
        ~LookAndFeel() override = default;

        void setColourChart(ColourChart const& colourChart) override;

        // juce::AlertWindow::LookAndFeelMethods
        void drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, juce::Rectangle<int> const& textArea, juce::TextLayout& textLayout) override;

        // juce::DocumentWindow::LookAndFeelMethods
        juce::Button* createDocumentWindowButton(int buttonType) override;
    };
} // namespace Application

ANALYSE_FILE_END
