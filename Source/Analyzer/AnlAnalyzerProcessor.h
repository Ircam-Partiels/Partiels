#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    std::unique_ptr<Vamp::Plugin> createPlugin(Accessor const& accessor, double sampleRate, AlertType alertType);
    
    std::vector<Analyzer::Result> performAnalysis(Accessor const& accessor, juce::AudioFormatReader& audioFormatReader);
}

ANALYSE_FILE_END
