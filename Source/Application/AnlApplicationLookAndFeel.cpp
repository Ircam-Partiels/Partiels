#include "AnlApplicationLookAndFeel.h"
#include "../Tools/AnlPropertyLayout.h"
#include "../Zoom/AnlZoomStateRuler.h"
#include "../Zoom/AnlZoomStateTimeRuler.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::ColouredPanel::ColourIds::backgroundColourId, juce::Colours::black);
    
    setColour(Zoom::State::Ruler::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Zoom::State::Ruler::tickColourId, juce::Colours::black);
    setColour(Zoom::State::Ruler::textColourId, juce::Colours::white);
    setColour(Zoom::State::Ruler::anchorColourId, juce::Colours::red);
    setColour(Zoom::State::Ruler::selectionColourId, juce::Colours::blue);
    setColour(Zoom::State::TimeRuler::backgroundColourId, juce::Colours::transparentBlack);
}

ANALYSE_FILE_END

