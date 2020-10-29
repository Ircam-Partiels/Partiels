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
        
        juce::TextButton mBackwardButton {juce::CharPointer_UTF8("«")};
        juce::TextButton mPlaybackButton {juce::CharPointer_UTF8("›")};
        juce::TextButton mForwardButton {juce::CharPointer_UTF8("»")};
        juce::TextButton mLoopButton {juce::CharPointer_UTF8("∞")};
        
        juce::Label mPlayPositionInSamples {"", "0samples"};
        juce::Label mPlayPositionInHMSms {"", "00h 00m 00s 000ms"};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Transport)
    };
}

ANALYSE_FILE_END
