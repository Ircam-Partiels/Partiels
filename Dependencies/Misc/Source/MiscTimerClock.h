#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

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

MISC_FILE_END
