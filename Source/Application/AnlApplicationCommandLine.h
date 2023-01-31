#pragma once

#include "../Document/AnlDocumentExecutor.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class CommandLine
    : public juce::ConsoleApplication
    {
    public:
        CommandLine();
        ~CommandLine();

        bool isRunning() const;

        static std::unique_ptr<CommandLine> createAndRun(juce::String const& commandLine);

    private:
        void runUnitTests();
        void compareFiles(juce::ArgumentList const& args);

        static void sendQuitSignal(int value);

        std::unique_ptr<Document::Executor> mExecutor;
        bool mShouldWait{false};
    };
} // namespace Application

ANALYSE_FILE_END
