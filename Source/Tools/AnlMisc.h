#pragma once

#define anlStrongAssert assert
#define anlWeakAssert jassert

#define ANALYSE_FILE_BEGIN namespace Anl {
    
#define ANALYSE_FILE_END }

#include "JuceHeader.h"

ANALYSE_FILE_BEGIN

namespace Logger
{
    inline void writeToLog(const char* level, const char* domain, const char* functionName, int line, juce::String const& message)
    {
        juce::Logger::writeToLog(juce::String("[") + level + "]" +
                                 "[" + domain + "]" +
                                 "[" + functionName + ":" + juce::String(line) + "] " +
                                 message);
    }
}

ANALYSE_FILE_END

#ifdef JUCE_DEBUG
#define AnlDebug(domain, message) Anl::Logger::writeToLog("Debug", domain, __FUNCTION__, __LINE__, message)
#define AnlError(domain, message) Anl::Logger::writeToLog("Error", domain, __FUNCTION__, __LINE__, message)
#else
#define AnlDebug(domain, message)
#define AnlError(domain, message)
#endif

#include <mutex>
#include <set>
#include <utility>
#include <atomic>
#include <memory>
#include <thread>

