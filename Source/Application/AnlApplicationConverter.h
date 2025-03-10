#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class ConverterContent
    : public juce::Component
    , public juce::FileDragAndDropTarget
    {
    public:
        ConverterContent();
        ~ConverterContent() override;

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

        void sdifAttributeUpdated();
        void selectedFrameUpdated();
        void selectedMatrixUpdated();
        void selectedRowColumnUpdated();

        void exportToJson();
        void exportToSdif();

        Accessor::Listener mListener{typeid(*this).name()};
        Document::Accessor::Listener mDocumentListener{typeid(*this).name()};

        PropertyTextButton mPropertyOpen;
        PropertyText mPropertyName;

        PropertyText mPropertyToSdifFrame;
        PropertyText mPropertyToSdifMatrix;
        PropertyText mPropertyToSdifColName;
        PropertyTextButton mPropertyToSdifExport;

        PropertyList mPropertyToJsonFrame;
        PropertyList mPropertyToJsonMatrix;
        PropertyList mPropertyToJsonRow;
        PropertyList mPropertyToJsonColumn;
        PropertyText mPropertyToJsonUnit;
        PropertyNumber mPropertyToJsonMinValue;
        PropertyNumber mPropertyToJsonMaxValue;
        PropertyToggle mPropertyToJsonLoadInDocument;
        PropertyTextButton mPropertyToJsonExport;
        juce::TextEditor mInfos;

        juce::File mFile;
        std::map<uint32_t, std::map<uint32_t, SdifConverter::matrix_size_t>> mEntries;
        std::vector<uint32_t> mFrameSigLinks;
        std::vector<uint32_t> mMatrixSigLinks;
        bool mFileIsDragging{false};
        std::unique_ptr<juce::FileChooser> mFileChooser;
    };

    class ConverterPanel
    : public HideablePanelTyped<ConverterContent>
    {
    public:
        ConverterPanel();
        ~ConverterPanel() override = default;
    };
} // namespace Application

ANALYSE_FILE_END
