
#include "AnlApplicationLookAndFeel.h"
#include "../Tools/AnlPropertyLayout.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::PropertyLayout::ColourIds::separatorColourId, juce::Colours::black);
//    setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
//    setColour(juce::Label::ColourIds::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
//    setColour(juce::Label::ColourIds::textWhenEditingColourId, juce::Colours::white);
}

ANALYSE_FILE_END

