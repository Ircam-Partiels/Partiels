#include "AnlTransportDisplay.h"

ANALYSE_FILE_BEGIN

Transport::Display::Display(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::playback:
            {
                auto const state = acsr.getAttr<AttrType::playback>();
                mPlaybackButton.setToggleState(state, juce::NotificationType::dontSendNotification);
                mPosition.setEnabled(!state);
                lookAndFeelChanged();
            }
            break;
            case AttrType::startPlayhead:
            {
                auto const state = acsr.getAttr<AttrType::playback>();
                if(!state)
                {
                    mPosition.setTime(acsr.getAttr<AttrType::startPlayhead>(), juce::NotificationType::dontSendNotification);
                }
            }
            break;
            case AttrType::runningPlayhead:
            {
                auto const state = acsr.getAttr<AttrType::playback>();
                if(state)
                {
                    mPosition.setTime(acsr.getAttr<AttrType::runningPlayhead>(), juce::NotificationType::dontSendNotification);
                }
            }
            break;
            case AttrType::looping:
            {
                auto const state = acsr.getAttr<AttrType::looping>();
                mLoopButton.setToggleState(state, juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::loopRange:
            case AttrType::gain:
            {
                auto const decibel = juce::Decibels::gainToDecibels(acsr.getAttr<AttrType::gain>(), -90.0);
                mVolumeSlider.setValue(decibel, juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mRewindButton.onClick = [&]()
    {
        auto const isPlaying = mAccessor.getAttr<Transport::AttrType::playback>();
        if(isPlaying)
        {
            mAccessor.setAttr<Transport::AttrType::playback>(false, NotificationType::synchronous);
        }
        mAccessor.setAttr<AttrType::startPlayhead>(0.0, NotificationType::synchronous);
        if(isPlaying)
        {
            mAccessor.setAttr<Transport::AttrType::playback>(true, NotificationType::synchronous);
        }
    };

    mPlaybackButton.setClickingTogglesState(true);
    mPlaybackButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::playback>(mPlaybackButton.getToggleState(), NotificationType::synchronous);
    };

    mLoopButton.setClickingTogglesState(true);
    mLoopButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::looping>(mLoopButton.getToggleState(), NotificationType::synchronous);
    };

    mVolumeSlider.setRange(-90.0, 12.0);
    mVolumeSlider.setDoubleClickReturnValue(true, 0.0);
    mVolumeSlider.onValueChange = [&]()
    {
        auto const gain = std::min(juce::Decibels::decibelsToGain(mVolumeSlider.getValue(), -90.0), 12.0);
        mAccessor.setAttr<AttrType::gain>(gain, NotificationType::synchronous);
    };

    mPosition.onTimeChanged = [&](double time)
    {
        mAccessor.setAttr<AttrType::startPlayhead>(time, NotificationType::synchronous);
    };

    addAndMakeVisible(mRewindButton);
    mRewindButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mPlaybackButton);
    mPlaybackButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mLoopButton);
    mLoopButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mPosition);
    mPosition.setWantsKeyboardFocus(false);

    addAndMakeVisible(mVolumeSlider);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Transport::Display::~Display()
{
    mAccessor.removeListener(mListener);
}

void Transport::Display::resized()
{
    auto bounds = getLocalBounds();
    auto const buttonSize = bounds.getHeight();

    mRewindButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(4));
    mPlaybackButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(4));
    mLoopButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(4));

    mVolumeSlider.setBounds(bounds.removeFromBottom(buttonSize / 3));
    mPosition.setBounds(bounds);
    mPosition.setFont(juce::Font(static_cast<float>(bounds.getHeight()) * 0.8f));
}

void Transport::Display::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mRewindButton, IconManager::IconType::rewind);
        laf->setButtonIcon(mLoopButton, IconManager::IconType::loop);
        auto const state = mAccessor.getAttr<AttrType::playback>();
        laf->setButtonIcon(mPlaybackButton, state ? IconManager::IconType::pause : IconManager::IconType::play);
    }
}

void Transport::Display::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

ANALYSE_FILE_END
