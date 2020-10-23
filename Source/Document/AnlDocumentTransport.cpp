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
                mPlayback.setToggleState(value, juce::NotificationType::dontSendNotification);
            }
                break;
            case Signal::toggleLooping:
            {
                mLoopButton.setToggleState(value, juce::NotificationType::dontSendNotification);
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
        mAccessor.sendSignal(Signal::toggleLooping, {mLoopButton.getToggleState()}, juce::NotificationType::sendNotificationSync);
    };
    
    addAndMakeVisible(mBackwardButton);
    addAndMakeVisible(mPlayback);
    addAndMakeVisible(mForwardButton);
    addAndMakeVisible(mLoopButton);
}

Document::Transport::~Transport()
{
    mAccessor.removeReceiver(mReceiver);
}

void Document::Transport::resized()
{
    auto bounds = getLocalBounds();
    auto const width = bounds.getWidth() / 4;
    mBackwardButton.setBounds(bounds.removeFromLeft(width));
    mPlayback.setBounds(bounds.removeFromLeft(width));
    mForwardButton.setBounds(bounds.removeFromLeft(width));
    mLoopButton.setBounds(bounds);
}

ANALYSE_FILE_END
