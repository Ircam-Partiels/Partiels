#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class FileWatcher
    : private juce::Timer
    {
    public:
        FileWatcher(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager);
        ~FileWatcher() override;
    private:
        
        // juce::Timer
        void timerCallback() override;
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        juce::Time mModificationTime;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWatcher)
    };
}

ANALYSE_FILE_END
