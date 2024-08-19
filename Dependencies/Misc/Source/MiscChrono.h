#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class Chrono
{
public:
    Chrono(juce::String const& domain, juce::String const& message);
    ~Chrono() = default;

#ifdef DEBUG
    void start();
    void stop();
#else
    inline void start() const noexcept
    {
    }
    inline void stop() const noexcept
    {
    }
#endif

private:
    juce::String const mDomain;
    juce::String const mMessage;
#ifdef DEBUG
    juce::int64 mTime;
#endif
};

MISC_FILE_END
