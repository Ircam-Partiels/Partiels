#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Transport
    : public juce::Component
    {
    public:
        using Signal = Model::Signal;
        
        Transport(Accessor& accessor);
        ~Transport() override;
        
        void resized() override;
    private:
        
        Accessor& mAccessor;
        Accessor::Receiver mReceiver;
        
        juce::TextButton mBackwardButton {"Backward"};
        juce::TextButton mPlayback {"Play/Stop"};
        juce::TextButton mForwardButton {"Forward"};
        juce::TextButton mLoopButton {"Loop"};
        
        JUCE_LEAK_DETECTOR(Transport)
    };
}

ANALYSE_FILE_END
