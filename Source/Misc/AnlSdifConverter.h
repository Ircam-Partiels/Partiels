#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

namespace SdifConverter
{
    static constexpr uint32_t getSignature(char const* const p)
    {
        return ((((static_cast<uint32_t>(p[0])) & 0xff) << 24) | (((static_cast<uint32_t>(p[1])) & 0xff) << 16) | (((static_cast<uint32_t>(p[2])) & 0xff) << 8) | ((static_cast<uint32_t>(p[3])) & 0xff));
    }

    // clang-format off
    enum SignatureIds : uint32_t
    {
          i1FQ0 = getSignature("1FQ0")
    };
    // clang-format on

    uint32_t getSignature(juce::String const& name);
    juce::String getString(uint32_t signature);

    // Return true to read the content, otherwise the content is ignored
    using FrameCallbackFn = std::function<bool(uint32_t frameSignature, size_t frameIndex, double time, size_t numMarix)>;
    using MatrixCallbackFn = std::function<bool(uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, size_t numRows, size_t numColumns, std::vector<std::string> columnNames)>;
    using DataCallbackFn = std::function<bool(uint32_t frameSignature, size_t frameIndex, uint32_t matrixSignature, double time, size_t numRows, size_t numColumns, std::variant<std::vector<std::string>, std::vector<std::vector<double>>> data)>;

    juce::Result read(juce::File const& file, FrameCallbackFn frameCallbackFn, MatrixCallbackFn matrixCallbackFn, DataCallbackFn dataCallbackFn);

    using Markers = std::vector<std::vector<std::tuple<double, double, std::string, std::vector<float>>>>;
    using Points = std::vector<std::vector<std::tuple<double, double, std::optional<float>, std::vector<float>>>>;
    using Columns = std::vector<std::vector<std::tuple<double, double, std::vector<float>, std::vector<float>>>>;

    juce::Result write(juce::File const& file, juce::Range<double> const& timeRange, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName, std::variant<Markers, Points, Columns> const& data, std::function<bool(void)> shouldContinue);

    using matrix_size_t = std::pair<size_t, std::vector<std::string>>;
    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> getEntries(juce::File const& file);
} // namespace SdifConverter

ANALYSE_FILE_END
