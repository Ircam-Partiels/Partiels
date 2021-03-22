#pragma once

#include "AnlTrackModel.h"
#include "AnlTrackProcessor.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Director
    {
    public:
        Director(Accessor& accessor, PluginList::Scanner& pluginListScanner, std::unique_ptr<juce::AudioFormatReader> audioFormatReader);
        ~Director();
        
        void setAudioFormatReader(std::unique_ptr<juce::AudioFormatReader> audioFormatReader, NotificationType const notification);
    private:
        
        void runAnalysis(NotificationType const notification);
        void updateZoomAccessors(NotificationType const notification);
        
        
        Accessor& mAccessor;
        PluginList::Scanner& mPluginListScanner;
        std::unique_ptr<juce::AudioFormatReader> mAudioFormatReaderManager;
        Processor mProcessor;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
