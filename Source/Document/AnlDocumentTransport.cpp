#include "AnlDocumentTransport.h"

ANALYSE_FILE_BEGIN

Document::Transport::Transport(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        switch (attribute)
        {
            case Attribute::gain:
            {
                auto const decibel = juce::Decibels::gainToDecibels(acsr.getModel().gain, -90.0);
                mVolumeSlider.setValue(decibel, juce::NotificationType::dontSendNotification);
            }
                break;
            case Attribute::isPlaybackStarted:
            {
                auto const state = mAccessor.getModel().isPlaybackStarted;
                mPlaybackButton.setButtonText(state ? juce::CharPointer_UTF8("□") : juce::CharPointer_UTF8("›"));
                mPlaybackButton.setToggleState(state, juce::NotificationType::dontSendNotification);
            }
                break;
            case Attribute::playheadPosition:
            {
                auto const samples = mAccessor.getModel().playheadPosition;
                mPlayPositionInSamples.setText(juce::String(samples) + " samples", juce::NotificationType::dontSendNotification);
            }
                break;
                
            default:
                break;
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, Signal signal, juce::var value)
    {
        juce::ignoreUnused(acsr);
        switch (signal)
        {
            case Signal::movePlayhead:
            {
            }
                break;
        }
    };
    mAccessor.addReceiver(mReceiver);
    
    mBackwardButton.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::movePlayhead, {false}, juce::NotificationType::sendNotificationSync);
    };
    mPlaybackButton.setClickingTogglesState(true);
    mPlaybackButton.onClick = [&]()
    {
        auto copy = mAccessor.getModel();
        copy.isPlaybackStarted = mPlaybackButton.getToggleState();
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
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
    
    mVolumeSlider.setRange(-90.0, 12.0);
    mVolumeSlider.setDoubleClickReturnValue(true, 0.0);
    mVolumeSlider.onValueChange = [&]()
    {
        auto copy = mAccessor.getModel();
        copy.gain = std::min(juce::Decibels::decibelsToGain(mVolumeSlider.getValue(), -90.0), 12.0);
        mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
    };
    
    addAndMakeVisible(mBackwardButton);
    addAndMakeVisible(mPlaybackButton);
    addAndMakeVisible(mForwardButton);
    addAndMakeVisible(mLoopButton);
    
    mPlayPositionInSamples.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mPlayPositionInSamples);
    mPlayPositionInHMSms.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mPlayPositionInHMSms);
    
    addAndMakeVisible(mVolumeSlider);
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
    mPlaybackButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    mForwardButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    mLoopButton.setBounds(topBounds.removeFromLeft(buttonWidth).reduced(4));
    
    mPlayPositionInSamples.setBounds(bounds.removeFromTop(bounds.getHeight() / 3));
    mPlayPositionInHMSms.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
    mVolumeSlider.setBounds(bounds);
}

ANALYSE_FILE_END
