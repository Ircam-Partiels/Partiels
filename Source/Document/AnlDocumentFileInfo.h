#pragma once

#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class FileInfoContent
    : public juce::Component
    {
    public:
        FileInfoContent(Director& director, juce::ApplicationCommandManager& commandManager);
        ~FileInfoContent() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;
        void parentHierarchyChanged() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        juce::ApplicationCommandManager& mApplicationCommandManager;

        PropertyLabel mNameLabel;
        PropertyLabel mDirectoryLabel;
        PropertyTextButton mRevealButton;
        ColouredPanel mSeparator;
        juce::Label mDescriptionLabel;
        juce::TextEditor mDescriptionEditor;

        JUCE_LEAK_DETECTOR(FileInfoContent)
    };

    class FileInfoPanel
    : public HideablePanel
    {
    public:
        FileInfoPanel(Director& director, juce::ApplicationCommandManager& commandManager);
        ~FileInfoPanel() override;

        // HideablePanel
        bool escapeKeyPressed() override;

    private:
        FileInfoContent mFileInfoContent;
    };
} // namespace Document

ANALYSE_FILE_END
