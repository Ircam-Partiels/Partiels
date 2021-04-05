#pragma once

#include "AnlTrackModel.h"
#include "AnlTrackProcessor.h"
#include "AnlTrackGraphics.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Director
    : private juce::Timer
    {
    public:
        Director(Accessor& accessor, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director() override;
        
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
    private:
        void runAnalysis(NotificationType const notification);
        void runRendering();
        
        // juce::Timer
        void timerCallback() override;
        
        Accessor& mAccessor;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        Processor mProcessor;
        Graphics mGraphics;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
