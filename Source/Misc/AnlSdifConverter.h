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

    juce::Result toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<size_t> row, std::optional<size_t> column, std::optional<nlohmann::json> const& extraInfo);
    juce::Result fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, std::optional<juce::String> columnName);

    class Panel
    : public juce::Component
    {
    public:
        Panel();
        ~Panel() override = default;

        // juce::Component
        void resized() override;

        void setFile(juce::File const& file);
        bool hasAnyChangeableOption() const;
        nlohmann::json getExtraInfo(double sampleRate) const;

        std::function<void(void)> onUpdated = nullptr;
        std::tuple<uint32_t, uint32_t, std::optional<juce::String>> getToSdifFormat() const;
        std::tuple<uint32_t, uint32_t, std::optional<size_t>, std::optional<size_t>> getFromSdifFormat() const;

    private:
        void selectedFrameUpdated();
        void selectedMatrixUpdated();
        void selectedRowColumnUpdated();
        void notify();

        PropertyText mPropertyToSdifFrame;
        PropertyText mPropertyToSdifMatrix;
        PropertyText mPropertyToSdifColName;

        PropertyList mPropertyToJsonFrame;
        PropertyList mPropertyToJsonMatrix;
        PropertyList mPropertyToJsonRow;
        PropertyList mPropertyToJsonColumn;

        juce::File mFile;
        std::map<uint32_t, std::map<uint32_t, SdifConverter::matrix_size_t>> mEntries;
        std::vector<uint32_t> mFrameSigLinks;
        std::vector<uint32_t> mMatrixSigLinks;
    };
} // namespace SdifConverter

ANALYSE_FILE_END
