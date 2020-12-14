#pragma once

#define ANALYSE_FILE_BEGIN namespace Anl {
    
#define ANALYSE_FILE_END }

#include "JuceHeader.h"

#include <mutex>
#include <set>
#include <utility>
#include <atomic>
#include <memory>
#include <thread>
#include <future>
#include <functional>
#include <type_traits>
#include <tuple>
#include <variant>
#include <numeric>
#include <cassert>

#define anlStrongAssert assert
#define anlWeakAssert jassert

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

namespace Tools
{
    inline juce::String secondsToString(double time)
    {
        auto const hours = static_cast<int>(std::floor(time / 3600.0));
        time -= static_cast<double>(hours) * 3600.0;
        auto const minutes = static_cast<int>(std::floor(time / 60.0));
        time -= static_cast<double>(minutes) * 60.0;
        auto const seconds = static_cast<int>(std::floor(time));
        time -= static_cast<double>(seconds);
        auto const ms = static_cast<int>(std::floor(time * 1000.0));
        return juce::String::formatted("%02dh %02dm %02ds %03dms", hours, minutes, seconds, ms);
    }
}

enum class NotificationType : bool
{
    synchronous = false,
    asynchronous = true
};

enum class AlertType : bool
{
    silent = false,
    window = true
};

ANALYSE_FILE_END

#ifdef JUCE_DEBUG
#define anlDebug(domain, message) Anl::Tools::Logger::writeToLog("Debug", domain, __FUNCTION__, __LINE__, message)
#define anlError(domain, message) Anl::Tools::Logger::writeToLog("Error", domain, __FUNCTION__, __LINE__, message)
#else
#define anlDebug(domain, message)
#define anlError(domain, message)
#endif

// This method can be used to test if a class/struct is a specialization of template class
// https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template<typename T, template<typename...> class Ref> struct is_specialization : std::false_type {};
template<template<typename...> class Ref, typename... Args> struct is_specialization<Ref<Args...>, Ref> : std::true_type {};
