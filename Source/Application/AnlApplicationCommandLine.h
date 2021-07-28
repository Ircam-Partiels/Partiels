#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class CommandLine
    : public juce::ConsoleApplication
    {
    public:
        CommandLine();
        ~CommandLine() = default;

        static std::optional<int> tryToRun(juce::String const& commandLine);
    };
} // namespace Application

ANALYSE_FILE_END
