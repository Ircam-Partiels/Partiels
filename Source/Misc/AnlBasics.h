#pragma once

#include "JuceHeader.h"

#include <atomic>
#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

// clang-format off
#define ANALYSE_FILE_BEGIN namespace Anl {
#define ANALYSE_FILE_END }

#define anlStrongAssert(condition) juce::ignoreUnused(condition); assert(condition)
#define anlWeakAssert jassert
// clang-format on

constexpr std::size_t operator""_z(unsigned long long n)
{
    return static_cast<std::size_t>(n);
}

ANALYSE_FILE_BEGIN

namespace Logger
{
    inline void writeToLog(const char* level, const char* domain, const char* functionName, int line, juce::String const& message)
    {
        juce::Logger::writeToLog(juce::String("[") + level + "]" + "[" + domain + "]" + "[" + functionName + ":" + juce::String(line) + "] " + message);
    }
} // namespace Logger

namespace Format
{
    inline juce::String secondsToString(double time, std::array<juce::StringRef, 4> const separators = {"h", "m", "s", "ms"})
    {
        auto const h = static_cast<int>(std::floor(time / 3600.0));
        time -= static_cast<double>(h) * 3600.0;
        auto const m = static_cast<int>(std::floor(time / 60.0));
        time -= static_cast<double>(m) * 60.0;
        auto const s = static_cast<int>(std::floor(time));
        time -= static_cast<double>(s);
        auto const ms = static_cast<int>(std::floor(time * 1000.0));
        return juce::String::formatted("%02d" + separators[0] + "%02d" + separators[1] + "%02d" + separators[2] + "%03d" + separators[3], h, m, s, ms);
    }

    inline std::string withFirstCharUpperCase(std::string text)
    {
        text[0_z] = static_cast<std::string::value_type>(std::toupper(static_cast<int>(text[0_z])));
        return text;
    }
} // namespace Format

namespace App
{
    inline juce::String getFileExtensionFor(juce::String const& suffix)
    {
        return juce::String(".") + APP_DOC_PREFIX + suffix;
    }

    inline juce::String getFileWildCardFor(juce::String const& suffix)
    {
        return "*" + getFileExtensionFor(suffix);
    }
} // namespace App

// clang-format off
enum class NotificationType : bool
{
      synchronous = false
    , asynchronous = true
};

enum class AlertType : bool
{
      silent = false
    , window = true
};

enum class ActionState
{
      abort
    , newTransaction
    , continueTransaction
};
// clang-format on

ANALYSE_FILE_END

#ifdef JUCE_DEBUG
#define anlDebug(domain, message) Anl::Logger::writeToLog("Debug", domain, __FUNCTION__, __LINE__, message)
#define anlError(domain, message) Anl::Logger::writeToLog("Error", domain, __FUNCTION__, __LINE__, message)
#else
#define anlDebug(domain, message) juce::ignoreUnused(domain, message);
#define anlError(domain, message) juce::ignoreUnused(domain, message);
#endif

// This method can be used to test if a class/struct is a specialization of template class
// https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template <typename T, template <typename...> class Ref>
struct is_specialization : std::false_type
{
};
template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type
{
};

// https://stackoverflow.com/questions/51408771/c-reversed-integer-sequence-implementation
template <std::size_t... Is>
constexpr auto index_sequence_reverse(std::index_sequence<Is...> const&) -> decltype(std::index_sequence<sizeof...(Is) - 1U - Is...>{});
template <std::size_t N>
using make_index_sequence_reverse = decltype(index_sequence_reverse(std::make_index_sequence<N>{}));

template <typename T>
std::vector<T> copy_with_erased(std::vector<T> const& v, T const& e)
{
    auto copy = v;
#ifdef __cpp_lib_erase_if
    std::erase(copy, e);
#else
    copy.erase(std::remove(copy.begin(), copy.end(), e), copy.end());
#endif
    return copy;
}

template <typename T, class Predicate>
std::vector<T> copy_with_erased_if(std::vector<T> const& v, Predicate predicate)
{
    auto copy = v;
#ifdef __cpp_lib_erase_if
    std::erase_if(copy, predicate);
#else
    copy.erase(std::remove_if(copy.begin(), copy.end(), predicate), copy.end());
#endif
    return copy;
}
