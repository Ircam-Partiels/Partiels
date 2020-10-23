#include "AnlDocumentTransport.h"

ANALYSE_FILE_BEGIN

Document::Transport::Transport(Accessor& accessor)
: mAccessor(accessor)
{
    mReceiver.onSignal = [&](Accessor& acsr, Signal signal, juce::var value)
    {
        switch (signal)
        {
            case Signal::movePlayhead:
            {
            }
                break;
            case Signal::togglePlayback:
            {
                mPlayback.setButtonText(value ? juce::CharPointer_UTF8("□") : juce::CharPointer_UTF8("›"));
                mPlayback.setToggleState(value, juce::NotificationType::dontSendNotification);
            }
                break;
            case Signal::playheadPosition:
            {
                auto const samples = static_cast<juce::int64>(value);
                mPlayPositionInSamples.setText(juce::String(samples) + " samples", juce::NotificationType::dontSendNotification);
            }
                break;
        }
    };
    mAccessor.addReceiver(mReceiver);
    
    mBackwardButton.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::movePlayhead, {false}, juce::NotificationType::sendNotificationSync);
    };
    mPlayback.setClickingTogglesState(true);
    mPlayback.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::togglePlayback, {mPlayback.getToggleState()}, juce::NotificationType::sendNotificationSync);
    };
    mForwardButton.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::movePlayhead, {true}, juce::NotificationType::sendNotificationSync);
    };
    mLoopButton.setClickingTogglesState(true);
    mLoopButton.onClick = [&]()
    {
        auto copy = mAccessor.getModel();
        copy.isLooping = mLoopButton.getToggleState();
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    };
    
    addAndMakeVisible(mBackwardButton);
    addAndMakeVisible(mPlayback);
    addAndMakeVisible(mForwardButton);
    addAndMakeVisible(mLoopButton);
    
    mPlayPositionInSamples.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mPlayPositionInSamples);
    mPlayPositionInHMSms.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mPlayPositionInHMSms);
}

Document::Transport::~Transport()
{
    mAccessor.removeReceiver(mReceiver);
}

void Document::Transport::resized()
{
    auto bounds = getLocalBounds();
    
    auto topBounds = bounds.removeFromTop(bounds.getHeight() / 3);
    auto const buttonWidth = topBounds.getWidth() / 4;
    mBackwardButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    mPlayback.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    mForwardButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    mLoopButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    
    mPlayPositionInSamples.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
    mPlayPositionInHMSms.setBounds(bounds);
}

ANALYSE_FILE_END
