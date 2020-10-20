#include "AnlColouredPanel.h"

ANALYSE_FILE_BEGIN

void Tools::ColouredPanel::ColouredPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId, true));
}


ANALYSE_FILE_END
