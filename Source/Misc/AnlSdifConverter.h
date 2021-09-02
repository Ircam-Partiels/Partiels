#pragma once

#include "AnlFloatingWindow.h"
#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

namespace SdifConverter
{
    uint32_t getSignature(juce::String const& name);

    using matrix_size_t = std::pair<size_t, std::vector<std::string>>;
    std::map<uint32_t, std::map<uint32_t, matrix_size_t>> getEntries(juce::File const& inputFile);

    juce::Result toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId, size_t row, std::optional<size_t> column);
    juce::Result fromJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId);

    class Panel
    : public FloatingWindowContainer
    , public juce::FileDragAndDropTarget
    {
    public:
        Panel();
        ~Panel() override = default;

        // juce::Component
        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;

        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;

    private:
        void setFile(juce::File const& file);

        void selectedFrameUpdated();
        void selectedMatrixUpdated();

        void exportToJson();
        void exportToSdif();

        PropertyTextButton mPropertyOpen;
        PropertyText mPropertyName;

        PropertyText mPropertyToSdifFrame;
        PropertyText mPropertyToSdifMatrix;
        PropertyTextButton mPropertyToSdifExport;

        PropertyList mPropertyToJsonFrame;
        PropertyList mPropertyToJsonMatrix;
        PropertyList mPropertyToJsonRow;
        PropertyList mPropertyToJsonColumn;
        PropertyTextButton mPropertyToJsonExport;
        juce::TextEditor mInfos;

        juce::File mFile;
        std::map<uint32_t, std::map<uint32_t, matrix_size_t>> mEntries;
        std::vector<uint32_t> mFrameSigLinks;
        std::vector<uint32_t> mMatrixSigLinks;
        bool mFileIsDragging{false};
        std::unique_ptr<juce::FileChooser> mFileChooser;
    };
} // namespace SdifConverter

ANALYSE_FILE_END
