#include "AnlColourSelector.h"

ANALYSE_FILE_BEGIN

ColourSelector::ColourSelector()
{
    addChangeListener(this);
}

ColourSelector::~ColourSelector()
{
    removeChangeListener(this);
}

void ColourSelector::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::ignoreUnused(source);
    anlWeakAssert(source == this);
    if(onColourChanged != nullptr)
    {
        onColourChanged(getCurrentColour());
    }
}
ANALYSE_FILE_END
