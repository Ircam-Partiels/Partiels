#pragma once

#include "AnlFloatingWindow.h"
#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

namespace SdifConverter
{
    uint32_t getSignature(juce::String const& name);
    juce::String getString(uint32_t signature);

    using matrix_size_t = std::pair<size_t, std::vector<std::string>>;
    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> getEntries(juce::File const& inputFile);

    juce::Result toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::optional<nlohmann::json> const& extraInfo);
    juce::Result fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName);
} // namespace SdifConverter

ANALYSE_FILE_END
