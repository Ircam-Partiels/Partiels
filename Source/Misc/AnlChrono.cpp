#include "AnlChrono.h"

ANALYSE_FILE_BEGIN

Chrono::Chrono(juce::String const& domain, juce::String const& message)
: mDomain(domain)
, mMessage(message)
{
}

void Chrono::start()
{
#ifdef DEBUG
    mTime = juce::Time::getHighResolutionTicks();
#endif
}

void Chrono::stop()
{
#ifdef DEBUG
    auto const duration = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - mTime);
    anlDebug(mDomain.getCharPointer(), mMessage + " " + juce::String(static_cast<int>(duration * 1000.0)) + "ms");
#endif
}

ANALYSE_FILE_END
