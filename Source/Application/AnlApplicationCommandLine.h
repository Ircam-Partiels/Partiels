#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace CommandLine
    {
        juce::Result analyzeAndExport(juce::File const& audioFile, juce::File const& templateFile, juce::File const& outputDir, juce::File const& optionsFile, juce::String const& identifier);
        int run(int argc, char* argv[]);
    } // namespace CommandLine
} // namespace Application

ANALYSE_FILE_END
