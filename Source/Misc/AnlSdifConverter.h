#pragma once

#include "AnlFloatingWindow.h"
#include "AnlPropertyComponent.h"

ANALYSE_FILE_BEGIN

namespace SdifConverter
{
    std::map<uint32_t, std::set<uint32_t>> getSignatures(juce::File const& inputFile);

    juce::Result toJson(juce::File const& inputFile, juce::File const& outputFile, uint32_t frameId, uint32_t matrixId);

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

        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;

    private:
        void setFile(juce::File const& file);
        void selectedFrameUpdated();
        void exportFile();

        PropertyTextButton mPropertyOpen;
        PropertyText mPropertyName;
        PropertyList mPropertyFrameId;
        PropertyList mPropertyMatrixId;
        PropertyTextButton mPropertyExport;
        bool mFileIsDragging{false};
        juce::File mFile;
        std::map<uint32_t, std::set<uint32_t>> mSignatures;
        std::vector<uint32_t> mFrameSigLinks;
        std::vector<uint32_t> mMatrixSigLinks;
    };
} // namespace SdifConverter

ANALYSE_FILE_END