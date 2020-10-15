
#include "AnlApplicationLookAndFeel.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    auto const greyColour = juce::Colours::grey;
    
    setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
    setColour(juce::Label::ColourIds::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::ColourIds::textWhenEditingColourId, juce::Colours::white);
}

ANALYSE_FILE_END

