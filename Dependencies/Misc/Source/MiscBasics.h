#pragma once

#include "JuceHeader.h"

#include <atomic>
#include <cassert>
#include <fstream>
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

#ifdef JUCE_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wfloat-equal"
#elif JUCE_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#include "../Dependencies/json/json.hpp"
#ifdef JUCE_CLANG
#pragma clang diagnostic pop
#elif JUCE_GCC
#pragma GCC diagnostic pop
#endif
#include "../Dependencies/magic_enum/magic_enum.hpp"

constexpr std::size_t operator""_z(unsigned long long n)
{
    return static_cast<std::size_t>(n);
}

// clang-format off
#define MISC_FILE_BEGIN namespace Misc {
#define MISC_FILE_END }
// clang-format on

#ifdef MISC_IGNORE_ASSERT
#define MiscStrongAssert(condition) juce::ignoreUnused(condition)
#define MiscWeakAssert(condition) juce::ignoreUnused(condition)
#else
#ifdef JUCE_DEBUG
#define MiscStrongAssert(condition) assert(condition)
#else
#define MiscStrongAssert(condition) juce::ignoreUnused(condition)
#endif
#define MiscWeakAssert jassert
#endif

#ifdef JUCE_DEBUG
#define MiscDebug(domain, message) Misc::Logger::writeToLog("Debug", domain, __FUNCTION__, __LINE__, message)
#define MiscError(domain, message) Misc::Logger::writeToLog("Error", domain, __FUNCTION__, __LINE__, message)
#else
#define MiscDebug(domain, message)
#define MiscError(domain, message)
#endif

#ifndef APP_DOC_PREFIX
#define APP_DOC_PREFIX ""
#endif

MISC_FILE_BEGIN

namespace Logger
{
    inline void writeToLog(const char* level, const char* domain, const char* functionName, int line, juce::String const& message)
    {
        juce::Logger::writeToLog(juce::String("[") + level + "]" + "[" + domain + "]" + "[" + functionName + ":" + juce::String(line) + "] " + message);
    }
} // namespace Logger

namespace Format
{
    inline juce::String secondsToString(double time, std::array<juce::StringRef, 4> const separators = {"h ", "m ", "s ", "ms"})
    {
        std::array<int, 4> values;
        values[0] = separators[0].isEmpty() ? 0 : static_cast<int>(std::floor(time / 3600.0));
        values[1] = separators[1].isEmpty() ? 0 : static_cast<int>(std::floor((time - static_cast<double>(values[0] * 3600)) / 60.0));
        values[2] = separators[2].isEmpty() ? 0 : static_cast<int>(std::floor(time - static_cast<double>(values[0] * 3600) - static_cast<double>(values[1] * 60)));
        values[3] = static_cast<int>(std::round(std::fmod(time * 1000.0, 1000.0)));
        juce::String text;
        for(size_t i = 0; i < 3; ++i)
        {
            if(separators[i].isNotEmpty())
            {
                text += juce::String::formatted("%02d" + separators[i], values[i]);
            }
        }
        return text + juce::String::formatted("%03d" + separators[3], values[3]);
    }

    inline std::string withFirstCharUpperCase(std::string text)
    {
        text[0_z] = static_cast<std::string::value_type>(std::toupper(static_cast<int>(text[0_z])));
        return text;
    }

    inline juce::String valueToString(double value, int numDecimals)
    {
        if(numDecimals == 0)
        {
            return juce::String(static_cast<int>(value));
        }
        return juce::String(value, numDecimals).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
    }

    inline juce::String valueToString(float value, int numDecimals)
    {
        if(numDecimals == 0)
        {
            return juce::String(static_cast<int>(value));
        }
        return juce::String(value, numDecimals).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
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

enum class ActionState
{
      abort
    , newTransaction
    , continueTransaction
};
// clang-format on

struct Version
{
    int major{0};
    int minor{0};
    int bugfix{0};

    juce::String toString() const
    {
        return juce::String(major) + "." + juce::String(minor) + "." + juce::String(bugfix);
    }

    const auto tie() const
    {
        return std::tie(major, minor, bugfix);
    }

    bool operator>(Version const& rhs) const
    {
        return tie() > rhs.tie();
    }

    bool operator<(Version const& rhs) const
    {
        return tie() < rhs.tie();
    }

    static Version fromString(juce::String const& text)
    {
        auto const numbers = juce::StringArray::fromTokens(text, ".", {});
        MiscWeakAssert(numbers.size() == 3);
        auto const major = numbers.size() >= 1 ? numbers[0].getIntValue() : 0;
        auto const minor = numbers.size() >= 2 ? numbers[1].getIntValue() : 0;
        auto const bugfix = numbers.size() >= 3 ? numbers[2].getIntValue() : 0;
        return {major, minor, bugfix};
    }
};

MISC_FILE_END

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

// https://timur.audio/using-locks-in-real-time-audio-processing-safely
struct audio_spin_mutex
{
    void lock() noexcept
    {
        if(!try_lock())
        {
            for(int i = 20; i >= 0; --i)
            {
                if(try_lock())
                {
                    return;
                }
            }

            while(!try_lock())
            {
                std::this_thread::yield();
            }
        }
    }

    bool try_lock() noexcept
    {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
