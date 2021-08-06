#pragma once

#include "AnlFloatingWindow.h"
#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

namespace SdifConverter
{
    juce::Result toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId);
} // namespace SdifConverter

ANALYSE_FILE_END
