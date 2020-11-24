#include "AnlDocumentTransport.h"

ANALYSE_FILE_BEGIN

Document::Transport::Transport(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::gain:
            {
                auto const decibel = juce::Decibels::gainToDecibels(acsr.getAttr<AttrType::gain>(), -90.0);
                mVolumeSlider.setValue(decibel, juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::isPlaybackStarted:
            {
                auto const state = acsr.getAttr<AttrType::isPlaybackStarted>();
                mPlaybackButton.setButtonText(state ? juce::CharPointer_UTF8("□") : juce::CharPointer_UTF8("›"));
                mPlaybackButton.setToggleState(state, juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::playheadPosition:
            {
                auto const time = acsr.getAttr<AttrType::playheadPosition>();
                mPlayPositionInHMSms.setText(Tools::secondsToString(time), juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::isLooping:
            {
                auto const state = acsr.getAttr<AttrType::isLooping>();
                mLoopButton.setToggleState(state, juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::file:
            case AttrType::timeZoom:
            case AttrType::analyzers:
                break;
        }
    };
    
    mBackwardButton.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::movePlayhead, {false}, NotificationType::synchronous);
    };
    mPlaybackButton.setClickingTogglesState(true);
    mPlaybackButton.onClick = [&]()
    {
        mAccessor.setValue<AttrType::isPlaybackStarted>(mPlaybackButton.getToggleState(), NotificationType::synchronous);
    };
    mForwardButton.onClick = [&]()
    {
        mAccessor.sendSignal(Signal::movePlayhead, {true}, NotificationType::synchronous);
    };
    mLoopButton.setClickingTogglesState(true);
    mLoopButton.onClick = [&]()
    {
        mAccessor.setValue<AttrType::isLooping>(mLoopButton.getToggleState(), NotificationType::synchronous);
    };
    
    mVolumeSlider.setRange(-90.0, 12.0);
    mVolumeSlider.setDoubleClickReturnValue(true, 0.0);
    mVolumeSlider.onValueChange = [&]()
    {
        auto const gain = std::min(juce::Decibels::decibelsToGain(mVolumeSlider.getValue(), -90.0), 12.0);
        mAccessor.setValue<AttrType::gain>(gain, NotificationType::synchronous);
    };
    
    addAndMakeVisible(mBackwardButton);
    addAndMakeVisible(mPlaybackButton);
    addAndMakeVisible(mForwardButton);
    addAndMakeVisible(mLoopButton);
    
    mPlayPositionInHMSms.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(mPlayPositionInHMSms);
    
    addAndMakeVisible(mVolumeSlider);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Transport::~Transport()
{
    mAccessor.removeListener(mListener);
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
    
    bounds.removeFromTop(bounds.getHeight() / 3);
    mPlayPositionInHMSms.setBounds(bounds.removeFromTop(bounds.getHeight() / 2));
    mVolumeSlider.setBounds(bounds);
}

ANALYSE_FILE_END
