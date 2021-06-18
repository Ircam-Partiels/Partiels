#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class TimerClock
: public juce::Timer
{
public:
    TimerClock() = default;
    ~TimerClock() override = default;

    std::function<void(void)> callback = nullptr;

protected:
    // juce::Timer
    void timerCallback() override;
};

ANALYSE_FILE_END
