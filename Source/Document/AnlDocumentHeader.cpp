#include "AnlDocumentHeader.h"
#include "AnlDocumentAudioReader.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::Header::Header(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mApplicationCommandManager(commandManager)
, mDocumentFileButton(juce::ImageCache::getFromMemory(AnlIconsData::documentfile_png, AnlIconsData::documentfile_pngSize))
, mReaderLayoutButton(juce::ImageCache::getFromMemory(AnlIconsData::audiolayer_png, AnlIconsData::audiolayer_pngSize))
, mTransportDisplay(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>(), commandManager)
, mTransportSelectionInfo(mAccessor.getAcsr<AcsrType::transport>(), mAccessor.getAcsr<AcsrType::timeZoom>())
, mOscButton(juce::translate("OSC"), juce::translate("Show OSC Settings"))
, mBubbleTooltipButton(juce::ImageCache::getFromMemory(AnlIconsData::bubble_png, AnlIconsData::bubble_pngSize))
, mEditModeButton(juce::ImageCache::getFromMemory(AnlIconsData::edit_png, AnlIconsData::edit_pngSize))
{
    addAndMakeVisible(mReaderLayoutButton);
    addAndMakeVisible(mDocumentFileButton);
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mTransportSelectionInfo);
    addAndMakeVisible(mOscButton);
    addAndMakeVisible(mBubbleTooltipButton);
    addAndMakeVisible(mEditModeButton);

    mDocumentFileButton.setWantsKeyboardFocus(false);
    mDocumentFileButton.onClick = [this]()
    {
        mAccessor.sendSignal(SignalType::showDocumentFilePanel, {}, NotificationType::synchronous);
    };
    mDocumentFileButton.setTooltip(juce::translate("Show document file panel"));

    mReaderLayoutButton.setWantsKeyboardFocus(false);
    mReaderLayoutButton.onClick = [this]()
    {
        mAccessor.sendSignal(SignalType::showReaderLayoutPanel, {}, NotificationType::synchronous);
    };

    mOscButton.setCommandToTrigger(&mApplicationCommandManager, ApplicationCommandIDs::helpOpenOscSettings, true);

    mBubbleTooltipButton.setClickingTogglesState(true);
    mBubbleTooltipButton.onClick = [this]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::viewInfoBubble, true);
    };

    mEditModeButton.setClickingTogglesState(true);
    mEditModeButton.onClick = [this]()
    {
        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::frameToggleDrawing, true);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const result = createAudioFormatReader(acsr, mDirector.getAudioFormatManager());
                auto const alertMessage = std::get<1>(result).joinIntoString("");
                if(alertMessage.isEmpty())
                {
                    mReaderLayoutButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::audiolayer_png, AnlIconsData::audiolayer_pngSize));
                    mReaderLayoutButton.setTooltip(juce::translate("Show audio files layout panel"));
                }
                else
                {
                    mReaderLayoutButton.setImages(juce::ImageCache::getFromMemory(AnlIconsData::alert_png, AnlIconsData::alert_pngSize));
                    mReaderLayoutButton.setTooltip(alertMessage);
                }
            }
            break;
            case AttrType::path:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
            case AttrType::description:
                break;
        }
    };

    mApplicationCommandManager.addListener(this);
    applicationCommandListChanged();
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(200, 48);
}

Document::Header::~Header()
{
    mAccessor.removeListener(mListener);
    mApplicationCommandManager.removeListener(this);
}

void Document::Header::resized()
{
    auto bounds = getLocalBounds();
    static auto constexpr spaceWidth = 4;
    static auto constexpr buttonHeight = 24;
    static auto constexpr transportDisplayWidth = 284;
    auto const sideWidth = std::max((bounds.getWidth() - transportDisplayWidth) / 2, 0);

    auto leftPart = bounds.removeFromLeft(sideWidth).withSizeKeepingCentre(sideWidth - spaceWidth * 2, buttonHeight);
    mDocumentFileButton.setVisible(leftPart.getWidth() >= 24);
    if(mDocumentFileButton.isVisible())
    {
        mDocumentFileButton.setBounds(leftPart.removeFromLeft(24));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }
    mReaderLayoutButton.setVisible(leftPart.getWidth() >= 24);
    if(mReaderLayoutButton.isVisible())
    {
        mReaderLayoutButton.setBounds(leftPart.removeFromLeft(24));
        leftPart = leftPart.withTrimmedLeft(spaceWidth);
    }

    auto rightPart = bounds.removeFromRight(sideWidth).reduced(spaceWidth, 0);
    auto const scrollbarWidth = getLookAndFeel().getDefaultScrollbarWidth();
    auto const hasRightButtons = rightPart.getWidth() >= 24 + scrollbarWidth;
    mBubbleTooltipButton.setVisible(hasRightButtons);
    mEditModeButton.setVisible(hasRightButtons);
    if(hasRightButtons)
    {
        auto rightButtonBounds = rightPart.removeFromRight(24 + scrollbarWidth).reduced((2 + scrollbarWidth) / 2, 4);
        mBubbleTooltipButton.setBounds(rightButtonBounds.removeFromTop(rightButtonBounds.getHeight() / 2).withSizeKeepingCentre(20, 20));
        mEditModeButton.setBounds(rightButtonBounds.withSizeKeepingCentre(20, 20));
    }
    mTransportSelectionInfo.setVisible(rightPart.getWidth() >= 164);
    if(mTransportSelectionInfo.isVisible())
    {
        mTransportSelectionInfo.setBounds(rightPart.removeFromRight(164));
    }
    mOscButton.setVisible(rightPart.getWidth() > 44);
    mOscButton.setBounds(rightPart.removeFromLeft(44).removeFromBottom(22).withSizeKeepingCentre(40, 16));

    mTransportDisplay.setVisible(bounds.getWidth() >= transportDisplayWidth);
    if(mTransportDisplay.isVisible())
    {
        bounds.removeFromLeft((bounds.getWidth() - transportDisplayWidth) / 2);
        mTransportDisplay.setBounds(bounds.removeFromLeft(transportDisplayWidth).withSizeKeepingCentre(transportDisplayWidth, 40));
    }
}

void Document::Header::colourChanged()
{
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::transportOscConnected});
}

void Document::Header::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::viewInfoBubble:
            mBubbleTooltipButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
            break;
        case ApplicationCommandIDs::frameToggleDrawing:
            mEditModeButton.setToggleState(info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked, juce::NotificationType::dontSendNotification);
            break;
        case ApplicationCommandIDs::transportOscConnected:
            mOscButton.setColour(juce::TextButton::ColourIds::buttonColourId, info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked ? juce::Colours::green : findColour(juce::TextButton::ColourIds::buttonColourId));
            break;
        default:
            break;
    }
}

void Document::Header::applicationCommandListChanged()
{
    mBubbleTooltipButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::viewInfoBubble));
    mEditModeButton.setTooltip(Utils::getCommandDescriptionWithKey(mApplicationCommandManager, ApplicationCommandIDs::frameToggleDrawing));
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::viewInfoBubble, ApplicationCommandIDs::frameToggleDrawing, ApplicationCommandIDs::transportOscConnected});
}

ANALYSE_FILE_END
