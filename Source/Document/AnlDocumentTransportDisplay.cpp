#include "AnlDocumentTransportDisplay.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::TransportDisplay::TransportDisplay(Transport::Accessor& accessor, Zoom::Accessor& zoomAcsr, juce::ApplicationCommandManager& commandManager)
: mTransportAccessor(accessor)
, mZoomAccessor(zoomAcsr)
, mApplicationCommandManager(commandManager)
, mRewindButton(juce::ImageCache::getFromMemory(AnlIconsData::rewind_png, AnlIconsData::rewind_pngSize))
, mPlaybackButton(juce::ImageCache::getFromMemory(AnlIconsData::play_png, AnlIconsData::play_pngSize), juce::ImageCache::getFromMemory(AnlIconsData::pause_png, AnlIconsData::pause_pngSize))
, mLoopButton(juce::ImageCache::getFromMemory(AnlIconsData::loop_png, AnlIconsData::loop_pngSize))
{
    mTransportListener.onAttrChanged = [&](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::playback:
            {
                auto const state = acsr.getAttr<Transport::AttrType::playback>();
                mPlaybackButton.setToggleState(state, juce::NotificationType::dontSendNotification);
                mRewindButton.setEnabled(state || Transport::Tools::canRewindPlayhead(acsr));
                mPosition.setEnabled(!state);
            }
            break;
            case Transport::AttrType::startPlayhead:
            {
                auto const state = acsr.getAttr<Transport::AttrType::playback>();
                if(!state)
                {
                    mPosition.setTime(acsr.getAttr<Transport::AttrType::startPlayhead>(), juce::NotificationType::dontSendNotification);
                }
                mRewindButton.setEnabled(Transport::Tools::canRewindPlayhead(acsr));
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                auto const state = acsr.getAttr<Transport::AttrType::playback>();
                if(state)
                {
                    mPosition.setTime(acsr.getAttr<Transport::AttrType::runningPlayhead>(), juce::NotificationType::dontSendNotification);
                }
            }
            break;
            case Transport::AttrType::looping:
            {
                auto const state = acsr.getAttr<Transport::AttrType::looping>();
                mLoopButton.setToggleState(state, juce::NotificationType::dontSendNotification);
                mRewindButton.setEnabled(Transport::Tools::canRewindPlayhead(acsr));
            }
            break;
            case Transport::AttrType::loopRange:
            {
                mRewindButton.setEnabled(Transport::Tools::canRewindPlayhead(acsr));
            }
            break;
            case Transport::AttrType::gain:
            {
                auto const decibel = juce::Decibels::gainToDecibels(acsr.getAttr<Transport::AttrType::gain>(), -90.0);
                mVolumeSlider.setValue(decibel, juce::NotificationType::dontSendNotification);
            }
            break;
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::selection:
                break;
        }
    };

    mZoomListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                mPosition.setRange(acsr.getAttr<Zoom::AttrType::globalRange>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mRewindButton.setCommandToTrigger(std::addressof(mApplicationCommandManager), ApplicationCommandIDs::transportRewindPlayHead, true);

    mPlaybackButton.setClickingTogglesState(true);
    mPlaybackButton.onClick = [&]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::transportTogglePlayback, true);
    };

    mLoopButton.setClickingTogglesState(true);
    mLoopButton.onClick = [&]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::transportToggleLooping, true);
    };

    mVolumeSlider.setTooltip(juce::translate("The volume of the audio playback"));
    mVolumeSlider.setRange(-90.0, 12.0);
    mVolumeSlider.setDoubleClickReturnValue(true, 0.0);
    mVolumeSlider.onValueChange = [&]()
    {
        auto const gain = std::min(juce::Decibels::decibelsToGain(mVolumeSlider.getValue(), -90.0), 12.0);
        mTransportAccessor.setAttr<Transport::AttrType::gain>(gain, NotificationType::synchronous);
    };

    mPosition.setTooltip("The time position of the playhead");
    mPosition.onTimeChanged = [&](double time)
    {
        mTransportAccessor.setAttr<Transport::AttrType::startPlayhead>(time, NotificationType::synchronous);
    };

    addAndMakeVisible(mRewindButton);
    addAndMakeVisible(mPlaybackButton);
    addAndMakeVisible(mLoopButton);
    addAndMakeVisible(mPosition);
    addAndMakeVisible(mVolumeSlider);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
    mZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
    mApplicationCommandManager.addListener(this);
    applicationCommandListChanged();
}

Document::TransportDisplay::~TransportDisplay()
{
    mApplicationCommandManager.removeListener(this);
    mZoomAccessor.removeListener(mZoomListener);
    mTransportAccessor.removeListener(mTransportListener);
}

void Document::TransportDisplay::resized()
{
    auto bounds = getLocalBounds();
    auto const buttonSize = bounds.getHeight();
    mRewindButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(2));
    mPlaybackButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(2));
    mLoopButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(2));
    mVolumeSlider.setBounds(bounds.removeFromBottom(buttonSize / 3));
    mPosition.setBounds(bounds);
    mPosition.setFont(juce::Font(static_cast<float>(bounds.getHeight()) * 0.8f));
}

void Document::TransportDisplay::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::transportTogglePlayback:
        {
            mPlaybackButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
        }
        break;
        case ApplicationCommandIDs::transportToggleLooping:
        {
            mLoopButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
        }
        break;
        default:
            break;
    }
}

void Document::TransportDisplay::applicationCommandListChanged()
{
    mPlaybackButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::transportTogglePlayback));
    mLoopButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::transportToggleLooping));
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::transportTogglePlayback, ApplicationCommandIDs::transportToggleLooping});
}

ANALYSE_FILE_END
