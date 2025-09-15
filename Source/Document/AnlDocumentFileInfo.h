#pragma once

#include "AnlDocumentDirector.h"
#include "../Application/AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class FileInfoContent
    : public juce::Component
    {
    public:
        FileInfoContent(Director& director);
        ~FileInfoContent() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};

        Icon mFileButton;
        juce::Label mDescriptionLabel;
        juce::TextEditor mDescriptionEditor;

        JUCE_LEAK_DETECTOR(FileInfoContent)
    };

    class FileInfoPanel
    : public HideablePanel
    {
    public:
        FileInfoPanel();
        ~FileInfoPanel() override;

        // HideablePanel
        bool escapeKeyPressed() override;

    private:
        FileInfoContent mFileInfoContent;
    };
} // namespace Document

ANALYSE_FILE_END