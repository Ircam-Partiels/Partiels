#pragma once

#include "../Document/AnlDocumentFileBased.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace FileManager
    {
        void newDocument();
        void openDocumentFile(juce::File const& file);
        void openAudioFiles(std::vector<juce::File> const& files);
        void openFiles(std::vector<juce::File> const& files);

        void saveDocument();
        void saveDocumentAs();
        void consolidateDocument();
        void saveAndQuit();
    } // namespace FileManager
} // namespace Application

ANALYSE_FILE_END
