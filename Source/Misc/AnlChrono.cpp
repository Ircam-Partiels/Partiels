#include "AnlChrono.h"

ANALYSE_FILE_BEGIN

Chrono::Chrono(juce::String const& domain, juce::String const& message)
: mDomain(domain)
, mMessage(message)
{
}

#ifdef DEBUG
void Chrono::start()
{
    mTime = juce::Time::getHighResolutionTicks();
}

void Chrono::stop()
{
    auto const duration = juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - mTime);
    anlDebug(mDomain.getCharPointer(), mMessage + " " + juce::String(static_cast<int>(duration * 1000.0)) + "ms");
}
#endif

ANALYSE_FILE_END
