#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentFileBased.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    {
    public:
        Director(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager);
        ~Director();
        
        void loadAudioFile(juce::File const& file, AlertType alertType);
        
    private:
        
        Accessor& mAccessor;
        juce::AudioFormatManager const& mAudioFormatManager;
        Accessor::Listener mListener;
        juce::Time mModificationTime;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
