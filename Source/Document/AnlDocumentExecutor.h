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

        juce::Result load(juce::File const& audioFile, juce::File const& templateFile, bool adaptOnSampleRate);
        juce::Result launch();
        juce::Result saveTo(juce::File const& outputFile);
        juce::Result exportTo(juce::File const& outputDir, juce::String const& filePrefix, Exporter::Options const& options, juce::String const& identifier);
        bool isRunning() const;

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
