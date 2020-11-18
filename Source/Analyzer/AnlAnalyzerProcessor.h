#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    std::unique_ptr<Vamp::Plugin> createPlugin(Accessor const& accessor, double sampleRate, bool showMessageOnFailure);
    void performAnalysis(Accessor& accessor, juce::AudioFormatReader& audioFormatReader, size_t blockSize = 512);
}

ANALYSE_FILE_END
