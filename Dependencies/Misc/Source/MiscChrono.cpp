#include "MiscChrono.h"

MISC_FILE_BEGIN

Chrono::Chrono(juce::String const& domain)
: mDomain(domain)
{
}

#ifdef DEBUG
void Chrono::start()
{
    mTime = juce::Time::getHighResolutionTicks();
}

void Chrono::stop(char const* message)
{
    auto const duration = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - mTime);
    MiscDebug(mDomain.getCharPointer(), juce::String(message) + " " + juce::String(static_cast<int>(duration * 1000.0)) + "ms");
}
#endif

MISC_FILE_END
