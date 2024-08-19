#include "MiscColouredPanel.h"

MISC_FILE_BEGIN

void ColouredPanel::ColouredPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId, true));
}

MISC_FILE_END
