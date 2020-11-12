#include "AnlApplicationLookAndFeel.h"
#include "../Tools/AnlPropertyLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Zoom/AnlZoomTimeRuler.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::ColouredPanel::ColourIds::backgroundColourId, juce::Colours::black);
    
    setColour(Zoom::Ruler::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Zoom::Ruler::tickColourId, juce::Colours::black);
    setColour(Zoom::Ruler::textColourId, juce::Colours::white);
    setColour(Zoom::Ruler::anchorColourId, juce::Colours::red);
    setColour(Zoom::Ruler::selectionColourId, juce::Colours::blue);
    setColour(Zoom::TimeRuler::backgroundColourId, juce::Colours::transparentBlack);
}

bool Application::LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
}

ANALYSE_FILE_END

