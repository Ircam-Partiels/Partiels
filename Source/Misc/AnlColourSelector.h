#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class ColourSelector
: public juce::ColourSelector
, private juce::ChangeListener
{
public:
    ColourSelector();
    ~ColourSelector() override;
    
    std::function<void(juce::Colour const& colour)> onColourChanged = nullptr;
private:
    // juce::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
};

ANALYSE_FILE_END
