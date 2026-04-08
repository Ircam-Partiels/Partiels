#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

class Chrono
{
public:
    Chrono(juce::String const& domain);
    ~Chrono() = default;

#if JUCE_DEBUG
    void start();
    void stop(char const* message);
#else
    inline void start() const noexcept
    {
    }
    inline void stop(char const*) const noexcept
    {
    }
#endif

private:
    juce::String const mDomain;
#if JUCE_DEBUG
    juce::int64 mTime;
#endif
};

MISC_FILE_END
