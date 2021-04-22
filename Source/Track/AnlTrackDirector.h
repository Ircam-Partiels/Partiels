#pragma once

#include "../Plugin/AnlPluginListScanner.h"
#include "AnlTrackGraphics.h"
#include "AnlTrackModel.h"
#include "AnlTrackProcessor.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Director
    : private juce::Timer
    {
    public:
        Director(Accessor& accessor, juce::UndoManager& undoManager, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;

        Accessor& getAccessor();

        void startAction();
        void endAction(juce::String const& name, ActionState state);

        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);

    private:
        void runAnalysis(NotificationType const notification);
        void runRendering();

        // juce::Timer
        void timerCallback() override;

        Accessor& mAccessor;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        Processor mProcessor;
        Graphics mGraphics;
        std::optional<std::reference_wrapper<Zoom::Accessor>> mSharedZoomAccessor;
        Zoom::Accessor::Listener mSharedZoomListener;
        std::mutex mSharedZoomMutex;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Track

ANALYSE_FILE_END
