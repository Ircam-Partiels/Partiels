#include "MiscTimerClock.h"

MISC_FILE_BEGIN

void TimerClock::timerCallback()
{
    if(callback != nullptr)
    {
        callback();
    }
}

MISC_FILE_END
