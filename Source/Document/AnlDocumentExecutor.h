#pragma once

#include "AnlDocumentDirector.h"
#include "AnlDocumentExporter.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Executor
    : private juce::Timer
    {
    public:
        Executor();
        ~Executor() override = default;

        //! @brief Loads a document from a file.
        juce::Result load(juce::File const& documentFile);

        //! @brief Loads a document from an audio file and a template file.
        juce::Result load(juce::File const& audioFile, juce::File const& templateFile, bool adaptOnSampleRate);

        //! @brief Runs the analysis.
        juce::Result launch();

        //! @brief Saves the current document to a file.
        juce::Result saveTo(juce::File const& outputFile);

        //! @brief Exports the results to a file.
        juce::Result exportTo(juce::File const& outputDir, juce::String const& filePrefix, Exporter::Options const& options, juce::String const& identifier);

        //! @brief Checks if the executor is currently running.
        bool isRunning() const;

        //! @brief The callback that is called when the analysis has ended.
        std::function<void(void)> onEnded = nullptr;

    private:
        // juce::Timer
        void timerCallback() override;

        juce::AudioFormatManager mAudioFormatManager;
        juce::UndoManager mUndoManager;
        Accessor mAccessor;
        Director mDirector{mAccessor, mAudioFormatManager, mUndoManager};
    };
} // namespace Document

ANALYSE_FILE_END
