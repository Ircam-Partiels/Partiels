#pragma once

#define anlStrongAssert assert
#define anlWeakAssert jassert

#define ANALYSE_FILE_BEGIN namespace Anl {
    
#define ANALYSE_FILE_END }

#include "JuceHeader.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
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
}

ANALYSE_FILE_END

#ifdef JUCE_DEBUG
#define AnlDebug(domain, message) Anl::Tools::Logger::writeToLog("Debug", domain, __FUNCTION__, __LINE__, message)
#define AnlError(domain, message) Anl::Tools::Logger::writeToLog("Error", domain, __FUNCTION__, __LINE__, message)
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
#include <functional>
#include <type_traits>
#include <tuple>

// This method can be used to test if a class is a specialization of template class
// https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template<typename Test, template<typename...> class Ref> struct is_specialization : std::false_type {};
template<template<typename...> class Ref, typename... Args> struct is_specialization<Ref<Args...>, Ref>: std::true_type {};
