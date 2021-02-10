#include "AnlDocumentTransport.h"

ANALYSE_FILE_BEGIN

Document::Transport::Transport(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
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
                mPlaybackButton.setToggleState(state, juce::NotificationType::dontSendNotification);
                lookAndFeelChanged();
            }
                break;
            case AttrType::playheadPosition:
            {
                auto const time = acsr.getAttr<AttrType::playheadPosition>();
                mPlayPositionInHMSms.setText(Format::secondsToString(time), juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::isLooping:
            {
                auto const state = acsr.getAttr<AttrType::isLooping>();
                mLoopButton.setToggleState(state, juce::NotificationType::dontSendNotification);
            }
                break;
            case AttrType::file:
            case AttrType::layoutHorizontal:
                break;
        }
    };
    
    mRewindButton.onClick = [&]()
    {
        auto const isPlaying = mAccessor.getAttr<Document::AttrType::isPlaybackStarted>();
        if(isPlaying)
        {
            mAccessor.setAttr<Document::AttrType::isPlaybackStarted>(false, NotificationType::synchronous);
        }
        mAccessor.setAttr<AttrType::playheadPosition>(0.0, NotificationType::synchronous);
        if(isPlaying)
        {
            mAccessor.setAttr<Document::AttrType::isPlaybackStarted>(true, NotificationType::synchronous);
        }
    };
    
    mPlaybackButton.setClickingTogglesState(true);
    mPlaybackButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::isPlaybackStarted>(mPlaybackButton.getToggleState(), NotificationType::synchronous);
    };
    
    mLoopButton.setClickingTogglesState(true);
    mLoopButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::isLooping>(mLoopButton.getToggleState(), NotificationType::synchronous);
    };
    
    mVolumeSlider.setRange(-90.0, 12.0);
    mVolumeSlider.setDoubleClickReturnValue(true, 0.0);
    mVolumeSlider.onValueChange = [&]()
    {
        auto const gain = std::min(juce::Decibels::decibelsToGain(mVolumeSlider.getValue(), -90.0), 12.0);
        mAccessor.setAttr<AttrType::gain>(gain, NotificationType::synchronous);
    };
    
    addAndMakeVisible(mRewindButton);
    addAndMakeVisible(mPlaybackButton);
    addAndMakeVisible(mLoopButton);
    
    mPlayPositionInHMSms.setJustificationType(juce::Justification::centredLeft);
    mPlayPositionInHMSms.setMinimumHorizontalScale(1.0f);
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
    auto const height = bounds.getHeight() / 4;
    auto const width = bounds.getWidth() / 3;
    auto topBounds = bounds.removeFromTop(height * 2);
    mRewindButton.setBounds(topBounds.removeFromLeft(width).reduced(4));
    mPlaybackButton.setBounds(topBounds.removeFromLeft(width).reduced(4));
    mLoopButton.setBounds(topBounds.removeFromLeft(width).reduced(4));
    mPlayPositionInHMSms.setBounds(bounds.removeFromTop(height));
    mVolumeSlider.setBounds(bounds);
}

void Document::Transport::lookAndFeelChanged()
{
    auto* lookAndFeel = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(lookAndFeel != nullptr);
    if(lookAndFeel != nullptr)
    {
        lookAndFeel->setButtonIcon(mRewindButton, IconManager::IconType::rewind);
        lookAndFeel->setButtonIcon(mLoopButton, IconManager::IconType::loop);
        auto const state = mAccessor.getAttr<AttrType::isPlaybackStarted>();
        lookAndFeel->setButtonIcon(mPlaybackButton, state ? IconManager::IconType::pause : IconManager::IconType::play);
    }
}

void Document::Transport::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

ANALYSE_FILE_END
