#include "AnlTimerClock.h"

ANALYSE_FILE_BEGIN

void TimerClock::timerCallback()
{
    if(callback != nullptr)
    {
        callback();
    }
}

ANALYSE_FILE_END
