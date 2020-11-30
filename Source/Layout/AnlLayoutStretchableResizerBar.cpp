#include "AnlLayoutStretchableResizerBar.h"

ANALYSE_FILE_BEGIN

void Layout::StretchableResizerBar::hasBeenMoved()
{
    if(onMoved != nullptr)
    {
        onMoved();
    }
    else
    {
        juce::StretchableLayoutResizerBar::hasBeenMoved();
    }
}

ANALYSE_FILE_END
